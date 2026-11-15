#include "WebViewWindow.h"
#include "WebView2EnvironmentOptions.h"
#include <iostream>
#include <vector>

using namespace Microsoft::WRL;

WebViewWindow::WebViewWindow() = default;
WebViewWindow::~WebViewWindow() = default;

static std::string WStringToString(const std::wstring& wstr) {
    if (wstr.empty()) return "";
    int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), nullptr, 0, nullptr, nullptr);
    std::string strTo(sizeNeeded, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], sizeNeeded, nullptr, nullptr);
    return strTo;
}
bool WebViewWindow::inicializar(HWND hwnd, const std::wstring& userDataFolder, 
                                const std::wstring& virtualHostName, const std::wstring& folderPath)
{
    std::cout << "[WebViewWindow] 1. Lanzando peticion asincrona de entorno..." << std::endl;

    auto options = Microsoft::WRL::Make<CoreWebView2EnvironmentOptions>();
    if (options) {
        options->put_AdditionalBrowserArguments(L"--autoplay-policy=no-user-gesture-required");
    }

    HRESULT hr = CreateCoreWebView2EnvironmentWithOptions(nullptr, userDataFolder.c_str(), options.Get(),
        Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [hwnd, virtualHostName, folderPath, this](HRESULT result, ICoreWebView2Environment* env) -> HRESULT
            {
                std::cout << "[WebViewWindow] 3. Entorno de WebView2 creado en segundo plano (Callback de Entorno)." << std::endl;
                if (FAILED(result) || !env) return result;

                env->CreateCoreWebView2Controller(hwnd, Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                    [hwnd, virtualHostName, folderPath, this](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT
                    {
                        std::cout << "[WebViewWindow] 4. Controlador y WebView listos y acoplados a la ventana (Callback de Controlador)." << std::endl;
                        if (SUCCEEDED(result) && controller)
                        {
                            m_controller = controller;
                            m_controller->get_CoreWebView2(&m_webView);

                            RECT bounds;
                            GetClientRect(hwnd, &bounds);
                            m_controller->put_Bounds(bounds);

                            if (!virtualHostName.empty() && !folderPath.empty())
                            {
                                ComPtr<ICoreWebView2_3> webView3;
                                if (SUCCEEDED(m_webView.As(&webView3)))
                                {
                                    webView3->SetVirtualHostNameToFolderMapping(
                                        virtualHostName.c_str(),
                                        folderPath.c_str(),
                                        COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND_ALLOW);
                                }
                            }
                            // ... registro de eventos ...
                        }
                        return S_OK;
                    }).Get());

                return std::cout << "[WebViewWindow] 3.5. Solicitud de controlador enviada." << std::endl, S_OK;
            }).Get());

    // ESTO PRUEBA LA ASINCRONIA:
    // Este mensaje aparecera en la consola ANTES de los pasos 3 y 4 de arriba,
    // demostrando que el hilo principal NO se quedo esperando.
    std::cout << "[WebViewWindow] 2. La funcion 'inicializar' TERMINO y devolvio el control al hilo principal INMEDIATAMENTE." << std::endl;

    return SUCCEEDED(hr);
}

void WebViewWindow::redimensionar(const RECT& bounds) {
    if (m_controller != nullptr) m_controller->put_Bounds(bounds);
}

void WebViewWindow::navegarATextoHtml(const std::string& htmlContent) {
    if (m_webView != nullptr) {
        std::wstring htmlW(htmlContent.begin(), htmlContent.end());
        m_webView->NavigateToString(htmlW.c_str());
    }
}

void WebViewWindow::registrarCallbackMensajes(MensajeCallback callback) {
    m_mensajeCallback = callback;
}