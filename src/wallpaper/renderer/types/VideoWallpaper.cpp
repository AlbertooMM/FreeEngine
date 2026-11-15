#include "VideoWallpaper.h"
#include <windows.h>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

// Compartidas con WallpaperRenderer y ImageWallpaper
extern WebViewWindow g_sharedWallpaperWebView;
extern bool g_sharedWebViewIniciado;

bool VideoWallpaper::inicializar(HWND hwndParent, const std::string& rutaArchivo) {
    m_hwndParent = hwndParent;
    m_rutaVideo = rutaArchivo;

    RECT rc;
    GetClientRect(m_hwndParent, &rc);

    std::string htmlContenido =
        "<html style='margin:0;padding:0;overflow:hidden;background:black;'>"
        "<body style='margin:0;padding:0;width:100vw;height:100vh;overflow:hidden;background:black;display:flex;justify-content:center;align-items:center;'>"
        "<video id='wallpaperVideo' src='http://app.engine" + m_rutaVideo + "' autoplay loop playsinline style='width:100vw;height:100vh;object-fit:cover;position:absolute;top:0;left:0;'></video>"
        "<script>"
        "  const vid = document.getElementById('wallpaperVideo');"
        "  vid.volume = 1.0;"
        "  window.pausarFondo = function() {"
        "    if(!vid.paused) { vid.pause(); }"
        "    vid.muted = true;"
        "  };"
        "  window.reanudarFondo = function() {"
        "    vid.muted = false;"
        "    if(vid.paused) { vid.play(); }"
        "  };"
        "  let playPromise = vid.play();"
        "  if (playPromise !== undefined) {"
        "    playPromise.catch(error => {"
        "      document.addEventListener('click', () => { vid.play(); }, {once: true});"
        "    });"
        "  }"
        "</script>"
        "</body></html>";

    if (!g_sharedWebViewIniciado) {
        wchar_t buffer[MAX_PATH];
        GetModuleFileNameW(nullptr, buffer, MAX_PATH);
        fs::path dirEjecutable = fs::path(buffer).parent_path();
        fs::path rutaPerfil = dirEjecutable / "WallpaperUserData";

        bool exitoInit = g_sharedWallpaperWebView.inicializar(
            m_hwndParent, 
            rutaPerfil.wstring(), 
            L"app.engine", 
            dirEjecutable.wstring()
        );

        if (!exitoInit) return false;
        g_sharedWebViewIniciado = true;
    }

    g_sharedWallpaperWebView.redimensionar(rc);

    if (g_sharedWallpaperWebView.obtenerWebView() != nullptr) {
        g_sharedWallpaperWebView.navegarATextoHtml(htmlContenido);
    } else {
        Sleep(100);
        if (g_sharedWallpaperWebView.obtenerWebView() != nullptr) {
            g_sharedWallpaperWebView.navegarATextoHtml(htmlContenido);
        }
    }

    return true;
}

void VideoWallpaper::actualizar() {
    RECT rc;
    GetClientRect(m_hwndParent, &rc);
    g_sharedWallpaperWebView.redimensionar(rc);
}

void VideoWallpaper::pausar() {
    if (g_sharedWallpaperWebView.obtenerWebView() != nullptr) {
        g_sharedWallpaperWebView.obtenerWebView()->ExecuteScript(L"if(window.pausarFondo) window.pausarFondo();", nullptr);
    }
}

void VideoWallpaper::reanudar() {
    if (g_sharedWallpaperWebView.obtenerWebView() != nullptr) {
        g_sharedWallpaperWebView.obtenerWebView()->ExecuteScript(L"if(window.reanudarFondo) window.reanudarFondo();", nullptr);
    }
}

void VideoWallpaper::liberar() {}