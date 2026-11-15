#pragma once

#include <windows.h>
#include <wrl.h>
#include "WebView2.h"
#include <string>
#include <functional>

class WebViewWindow {
public:
    WebViewWindow();
    ~WebViewWindow();

    // Ahora inicializa de forma puramente asíncrona sin bloquear el hilo principal
    bool inicializar(HWND hwnd, const std::wstring& userDataFolder, 
                     const std::wstring& virtualHostName = L"", 
                     const std::wstring& folderPath = L"");

    void redimensionar(const RECT& bounds);
    void navegarATextoHtml(const std::string& htmlContent);

    using MensajeCallback = std::function<void(const std::string&)>;
    void registrarCallbackMensajes(MensajeCallback callback);

    Microsoft::WRL::ComPtr<ICoreWebView2> obtenerWebView() const { return m_webView; }
    Microsoft::WRL::ComPtr<ICoreWebView2Controller> obtenerController() const { return m_controller; }

private:
    Microsoft::WRL::ComPtr<ICoreWebView2Controller> m_controller;
    Microsoft::WRL::ComPtr<ICoreWebView2> m_webView;
    MensajeCallback m_mensajeCallback;
};