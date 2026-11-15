#include "WebViewManager.h"
#include "ActiveWallpaper.h"
#include "WallpaperRenderer.h"
#include "DesktopManager.h"
#include "UpdaterManager.h"
#include <windows.h>
#include <shellapi.h>
#include <wrl.h>
#include "WebView2.h"
#include "WebView2EnvironmentOptions.h"
#include <iostream>
#include <algorithm>
#include <filesystem>
#include <thread>
#include <fstream>

using namespace Microsoft::WRL;
namespace fs = std::filesystem;

// Versión actual de la aplicación
const std::string VERSION_ACTUAL = "v1.0.0";

// Variables globales para la ventana, el controlador, el WebView y la URL pendiente
static HWND g_hwnd = nullptr;
static ComPtr<ICoreWebView2Controller> g_webController = nullptr;
static ComPtr<ICoreWebView2> g_webView = nullptr;
static std::wstring g_urlDescargaPendiente = L"";

// Mensajes y constantes para la bandeja del sistema (System Tray)
#define WM_MOSTRAR_ACTUALIZACION (WM_USER + 101)
#define WM_TRAYICON              (WM_USER + 102)
#define ID_TRAY_ICON             1001
#define ID_MENU_ABRIR            40001
#define ID_MENU_SALIR            40002

// Declaración anticipada del procedimiento de ventana y función de creación
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void CrearVentanaPanelControl();

// Funciones para gestionar el icono en la bandeja del sistema
void AgregarIconoTray(HWND hwnd) 
{
    NOTIFYICONDATAW nid = { sizeof(NOTIFYICONDATAW) };
    nid.hWnd = hwnd;
    nid.uID = ID_TRAY_ICON;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = LoadIcon(nullptr, IDI_APPLICATION); 
    wcscpy_s(nid.szTip, L"FreeEngine - Wallpaper Engine");

    Shell_NotifyIconW(NIM_ADD, &nid);
}

void EliminarIconoTray(HWND hwnd) 
{
    NOTIFYICONDATAW nid = { sizeof(NOTIFYICONDATAW) };
    nid.hWnd = hwnd;
    nid.uID = ID_TRAY_ICON;
    Shell_NotifyIconW(NIM_DELETE, &nid);
}

// Función para descargar y aplicar la actualización automáticamente
void ejecutarActualizacionAutomatica(const std::string& urlDescarga)
{
    std::cout << "[Actualizador] Iniciando proceso de actualización automática..." << std::endl;

    wchar_t bufferExe[MAX_PATH];
    GetModuleFileNameW(nullptr, bufferExe, MAX_PATH);
    fs::path dirApp = fs::path(bufferExe).parent_path();
    fs::path tempZip = fs::temp_directory_path() / "FreeEngine_Update.zip";
    fs::path tempBat = fs::temp_directory_path() / "actualizar_freeengine.bat";

    std::wstring urlW(urlDescarga.begin(), urlDescarga.end());
    std::wstring comandoDownload = L"powershell -Command \"Invoke-WebRequest -Uri '" + urlW + L"' -OutFile '" + tempZip.wstring() + L"'\"";
    
    int resultadoDownload = _wsystem(comandoDownload.c_str());
    if (resultadoDownload != 0 || !fs::exists(tempZip)) {
        std::cout << "[Actualizador] ERROR: No se pudo descargar el archivo de actualización." << std::endl;
        return;
    }

    std::ofstream batFile(tempBat);
    if (!batFile.is_open()) return;

    batFile << "@echo off\n";
    batFile << "timeout /t 1 /nobreak >nul\n";
    batFile << "taskkill /f /im FreeEngine.exe >nul 2>&1\n";
    batFile << "powershell -Command \"Expand-Archive -Path '" << tempZip.string() << "' -DestinationPath '" << dirApp.string() << "' -Force\"\n";
    batFile << "del \"" << tempZip.string() << "\"\n";
    batFile << "start \"\" \"" << (dirApp / "FreeEngine.exe").string() << "\"\n";
    batFile << "del \"%~f0\"\n";
    batFile.close();

    ShellExecuteW(nullptr, L"open", tempBat.wstring().c_str(), nullptr, nullptr, SW_HIDE);
    PostQuitMessage(0);
}

void iniciarVerificacionActualizaciones(ICoreWebView2* webView)
{
    std::thread([]()
    {
        InfoActualizacion info = UpdaterManager::comprobarActualizaciones(L"AlbertooMM/FreeEngine", VERSION_ACTUAL);
        if (info.hayNuevaVersio) {
            g_urlDescargaPendiente = std::wstring(info.urlDescarga.begin(), info.urlDescarga.end());
            if (g_hwnd) {
                PostMessageW(g_hwnd, WM_MOSTRAR_ACTUALIZACION, 0, 0);
            }
        }
    }).detach();
}

// Función para crear la ventana y cargar WebView2
void CrearVentanaPanelControl()
{
    if (g_hwnd != nullptr) {
        // Si ya existe, solo la traemos al frente
        ShowWindow(g_hwnd, SW_SHOW);
        SetForegroundWindow(g_hwnd);
        return;
    }

    HINSTANCE hInstance = GetModuleHandle(nullptr);

    // 1. Registrar clase si no está registrada
    WNDCLASSEXW wc = {sizeof(WNDCLASSEXW)};
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"FreeEngineWindowClass";
    RegisterClassExW(&wc);

    // 2. Crear la ventana desde cero (libera memoria limpia al abrirse)
    g_hwnd = CreateWindowExW(
        0, L"FreeEngineWindowClass", L"FreeEngine - Panel de Control",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 1280, 720,
        nullptr, nullptr, hInstance, nullptr);

    if (!g_hwnd) return;

    auto options = Microsoft::WRL::Make<CoreWebView2EnvironmentOptions>();
    if (options) {
        options->put_AdditionalBrowserArguments(L"--autoplay-policy=no-user-gesture-required");
    }

    // 3. Inicializar WebView2
    CreateCoreWebView2EnvironmentWithOptions(nullptr, nullptr, options.Get(),
        Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [](HRESULT result, ICoreWebView2Environment *env) -> HRESULT
            {
                if (FAILED(result)) return result;

                env->CreateCoreWebView2Controller(g_hwnd, Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                    [](HRESULT result, ICoreWebView2Controller *controller) -> HRESULT
                    {
                        if (FAILED(result) || !controller) return result;

                        g_webController = controller;
                        g_webController->get_CoreWebView2(&g_webView);

                        RECT bounds;
                        GetClientRect(g_hwnd, &bounds);
                        g_webController->put_Bounds(bounds);

                        wchar_t buffer[MAX_PATH];
                        GetModuleFileNameW(nullptr, buffer, MAX_PATH);
                        fs::path dirProyecto = fs::path(buffer).parent_path();
                        ComPtr<ICoreWebView2_3> webView3;
                        if (SUCCEEDED(g_webView.As(&webView3))) {
                            webView3->SetVirtualHostNameToFolderMapping(
                                L"app.engine",
                                dirProyecto.wstring().c_str(),
                                COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND_ALLOW);
                        }

                        // Interceptar mensajes desde JavaScript
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

                                    if (!jsonStr.empty() && jsonStr.front() == '"' && jsonStr.back() == '"') {
                                        jsonStr = jsonStr.substr(1, jsonStr.size() - 2);
                                    }
                                    jsonStr.erase(std::remove(jsonStr.begin(), jsonStr.end(), '\\'), jsonStr.end());

                                    if (jsonStr.find("\"accion\":\"actualizar\"") != std::string::npos) {
                                        size_t posUrl = jsonStr.find("\"url\":\"");
                                        if (posUrl != std::string::npos) {
                                            size_t inicio = posUrl + 7;
                                            size_t fin = jsonStr.find("\"", inicio);
                                            if (fin != std::string::npos) {
                                                std::string urlDescarga = jsonStr.substr(inicio, fin - inicio);
                                                std::thread([urlDescarga]() { ejecutarActualizacionAutomatica(urlDescarga); }).detach();
                                            }
                                        }
                                        return S_OK;
                                    }

                                    std::string idFondo = "";
                                    int monitorIndex = 0;

                                    size_t posId = jsonStr.find("\"id\":\"");
                                    if (posId != std::string::npos) {
                                        size_t inicio = posId + 6;
                                        size_t fin = jsonStr.find("\"", inicio);
                                        if (fin != std::string::npos) {
                                            idFondo = jsonStr.substr(inicio, fin - inicio);
                                        }
                                    }

                                    size_t posMon = jsonStr.find("\"monitor\":");
                                    if (posMon != std::string::npos) {
                                        size_t inicio = posMon + 10;
                                        monitorIndex = std::stoi(jsonStr.substr(inicio));
                                    }

                                    if (!idFondo.empty()) {
                                        std::cout << "[PanelControl] Fondo seleccionado: " << idFondo << " | Monitor objetivo: " << monitorIndex << std::endl;
                                        ActiveWallpaper::guardarFondoActivo(idFondo, monitorIndex);
                                        WallpaperRenderer::actualizarFondoActivo();
                                    }

                                    return S_OK;
                                })
                            .Get(),
                            nullptr);

                        g_webView->add_NavigationCompleted(
                            Callback<ICoreWebView2NavigationCompletedEventHandler>(
                                [](ICoreWebView2 *sender, ICoreWebView2NavigationCompletedEventArgs *args) -> HRESULT
                                {
                                    BOOL isSuccess = FALSE;
                                    args->get_IsSuccess(&isSuccess);
                                    if (isSuccess) {
                                        iniciarVerificacionActualizaciones(sender);
                                    }
                                    return S_OK;
                                })
                            .Get(),
                            nullptr);

                        g_webView->Navigate(L"http://app.engine/menu/index.html");
                        return S_OK;
                    })
                .Get());
                return S_OK;
            })
        .Get());
}

void WebViewManager::iniciarVentana(const std::string &tituloVentana, const std::string &rutaHtml)
{
    // Creamos el panel de control al arrancar
    CrearVentanaPanelControl();

    // Bucle principal de mensajes de Windows (se mantiene vivo aunque la ventana se destruya)
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
    case WM_CREATE:
        AgregarIconoTray(hwnd);
        break;

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
    case WM_MOSTRAR_ACTUALIZACION:
    {
        if (g_webView != nullptr && !g_urlDescargaPendiente.empty())
        {
            std::wstring jsScript = L"mostrarIconoActualizacionVerde('" + g_urlDescargaPendiente + L"');";
            g_webView->ExecuteScript(jsScript.c_str(), nullptr);
        }
        break;
    }
    case WM_TRAYICON:
    {
        if (LOWORD(lParam) == WM_RBUTTONUP || LOWORD(lParam) == WM_LBUTTONUP || LOWORD(lParam) == NIN_SELECT)
        {
            POINT pt;
            GetCursorPos(&pt);
            SetForegroundWindow(hwnd);

            HMENU hMenu = CreatePopupMenu();
            AppendMenuW(hMenu, MF_STRING, ID_MENU_ABRIR, L"Abrir Panel de Control");
            AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
            AppendMenuW(hMenu, MF_STRING, ID_MENU_SALIR, L"Salir de FreeEngine");

            TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hwnd, nullptr);
            DestroyMenu(hMenu);
        }
        break;
    }
    case WM_COMMAND:
    {
        int id = LOWORD(wParam);
        if (id == ID_MENU_ABRIR)
        {
            // Recrear o mostrar la ventana del panel destruida previamente
            CrearVentanaPanelControl();
        }
        else if (id == ID_MENU_SALIR)
        {
            EliminarIconoTray(hwnd);
            PostQuitMessage(0);
        }
        break;
    }
    case WM_CLOSE:
    {
        // ¡DESTRUIR LA VENTANA PARA LIBERAR RAM Y CPU POR COMPLETO!
        DestroyWindow(hwnd);
        return 0; 
    }
    case WM_DESTROY:
    {
        // Limpiamos los punteros de la interfaz al destruirla
        EliminarIconoTray(hwnd);
        g_webController = nullptr;
        g_webView = nullptr;
        g_hwnd = nullptr; // Indicamos que ya no hay ventana activa
        break;
    }
    case WM_KEYDOWN:
    {
        if (wParam == VK_F12 && g_webView != nullptr)
        {
            g_webView->OpenDevToolsWindow();
        }
        break;
    }
    default:
        return DefWindowProc(hwnd, message, wParam, lParam);
    }
    return 0;
}