#include "WebViewManager.h"
#include "ActiveWallpaper.h"
#include "WallpaperRenderer.h"
#include "DesktopManager.h"
#include <windows.h>
#include <wrl.h>
#include "WebView2.h"
#include "WebView2EnvironmentOptions.h"
#include <iostream>
#include <algorithm>
#include <filesystem>

using namespace Microsoft::WRL;
namespace fs = std::filesystem;

// Variables globales para la ventana, el controlador y el WebView
static HWND g_hwnd = nullptr;
static ComPtr<ICoreWebView2Controller> g_webController = nullptr;
static ComPtr<ICoreWebView2> g_webView = nullptr;

// Declaración anticipada del procedimiento de ventana
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

void WebViewManager::iniciarVentana(const std::string &tituloVentana, const std::string &rutaHtml)
{
    HINSTANCE hInstance = GetModuleHandle(nullptr);

    // 1. Registrar la clase de ventana
    WNDCLASSEXW wc = {sizeof(WNDCLASSEXW)};
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"FreeEngineWindowClass";
    RegisterClassExW(&wc);

    // 2. Crear la ventana principal como una aplicación normal
    std::wstring tituloW(tituloVentana.begin(), tituloVentana.end());
    g_hwnd = CreateWindowExW(
        0, L"FreeEngineWindowClass", tituloW.c_str(),
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 1280, 720,
        nullptr, nullptr, hInstance, nullptr);

    if (!g_hwnd)
        return;

    // --- CONFIGURAR OPCIONES DE CHROMIUM (AUTOPLAY CON SONIDO) ---
    auto options = Microsoft::WRL::Make<CoreWebView2EnvironmentOptions>();
    if (options)
    {
        options->put_AdditionalBrowserArguments(L"--autoplay-policy=no-user-gesture-required");
    }

    // 3. Inicializar entorno de WebView2 pasando las opciones
    CreateCoreWebView2EnvironmentWithOptions(nullptr, nullptr, options.Get(),
        Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [](HRESULT result, ICoreWebView2Environment *env) -> HRESULT
            {
                if (FAILED(result))
                    return result;

                env->CreateCoreWebView2Controller(g_hwnd, Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                    [](HRESULT result, ICoreWebView2Controller *controller) -> HRESULT
                    {
                        if (FAILED(result) || !controller)
                            return result;

                        g_webController = controller;
                        g_webController->get_CoreWebView2(&g_webView);

                        // Ajustar tamaño inicial al área de cliente de la ventana
                        RECT bounds;
                        GetClientRect(g_hwnd, &bounds);
                        g_webController->put_Bounds(bounds);

                        // --- MAPEO DE CARPETA VIRTUAL (Raíz del proyecto: menu/ y assets/) ---
                        wchar_t buffer[MAX_PATH];
                        GetModuleFileNameW(nullptr, buffer, MAX_PATH);
                        fs::path dirProyecto = fs::path(buffer).parent_path();
                        ComPtr<ICoreWebView2_3> webView3;
                        if (SUCCEEDED(g_webView.As(&webView3)))
                        {
                            HRESULT mapResult = webView3->SetVirtualHostNameToFolderMapping(
                                L"app.engine",
                                dirProyecto.wstring().c_str(),
                                COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND_ALLOW);

                            if (SUCCEEDED(mapResult))
                            {
                                std::cout << "[WebView] Host virtual mapeado correctamente a la raiz: " << dirProyecto.string() << std::endl;
                            }
                        }

                        // Interceptar mensajes desde JavaScript (postMessage) recibiendo JSON {id, monitor}
                        g_webView->add_WebMessageReceived(
                            Callback<ICoreWebView2WebMessageReceivedEventHandler>(
                                [](ICoreWebView2 *sender, ICoreWebView2WebMessageReceivedEventArgs *args) -> HRESULT
                                {
                                    PWSTR message;
                                    args->get_WebMessageAsJson(&message);
                                    std::wstring ws(message);
                                    CoTaskMemFree(message);

                                    int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), (int)ws.size(), NULL, 0, NULL, NULL);
                                    std::string jsonStr(sizeNeeded, 0);
                                    WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), (int)ws.size(), &jsonStr[0], sizeNeeded, NULL, NULL);

                                    // Limpiar comillas y caracteres de escape del JSON enviado por WebView2
                                    if (!jsonStr.empty() && jsonStr.front() == '"' && jsonStr.back() == '"')
                                    {
                                        jsonStr = jsonStr.substr(1, jsonStr.size() - 2);
                                    }
                                    jsonStr.erase(std::remove(jsonStr.begin(), jsonStr.end(), '\\'), jsonStr.end());

                                    // Parsear id y monitor de forma sencilla
                                    std::string idFondo = "";
                                    int monitorIndex = 0;

                                    size_t posId = jsonStr.find("\"id\":\"");
                                    if (posId != std::string::npos)
                                    {
                                        size_t inicio = posId + 6;
                                        size_t fin = jsonStr.find("\"", inicio);
                                        if (fin != std::string::npos)
                                        {
                                            idFondo = jsonStr.substr(inicio, fin - inicio);
                                        }
                                    }

                                    size_t posMon = jsonStr.find("\"monitor\":");
                                    if (posMon != std::string::npos)
                                    {
                                        size_t inicio = posMon + 10;
                                        monitorIndex = std::stoi(jsonStr.substr(inicio));
                                    }

                                    std::cout << "[PanelControl] Fondo seleccionado: " << idFondo << " | Monitor objetivo: " << monitorIndex << std::endl;

                                    // Guardar en activo.json sin cerrar la ventana
                                    ActiveWallpaper::guardarFondoActivo(idFondo, monitorIndex);
                                    WallpaperRenderer::actualizarFondoActivo();

                                    return S_OK;
                                })
                            .Get(),
                            nullptr);

                        // Detectar estado de navegación
                        g_webView->add_NavigationCompleted(
                            Callback<ICoreWebView2NavigationCompletedEventHandler>(
                                [](ICoreWebView2 *sender, ICoreWebView2NavigationCompletedEventArgs *args) -> HRESULT
                                {
                                    BOOL isSuccess = FALSE;
                                    args->get_IsSuccess(&isSuccess);
                                    if (isSuccess)
                                    {
                                        std::cout << "[WebView] Menu cargado exitosamente en el panel de control." << std::endl;
                                    }
                                    return S_OK;
                                })
                            .Get(),
                            nullptr);

                        // Navegar a la interfaz web
                        g_webView->Navigate(L"http://app.engine/menu/index.html");

                        return S_OK;
                    })
                .Get());

                return S_OK;
            })
        .Get());

    // 4. Bucle de mensajes de Windows
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_SIZE:
    {
        if (g_webController != nullptr)
        {
            RECT bounds;
            GetClientRect(hwnd, &bounds);
            g_webController->put_Bounds(bounds);
        }
        break;
    }
    case WM_KEYDOWN:
    {
        // Abrir herramientas de desarrollo (DevTools) al pulsar F12
        if (wParam == VK_F12)
        {
            if (g_webView != nullptr)
            {
                g_webView->OpenDevToolsWindow();
                std::cout << "[WebView] Abriendo consola DevTools..." << std::endl;
            }
        }
        break;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, message, wParam, lParam);
    }
    return 0;
}