#pragma once
#include "../WallpaperBase.h"
#include "../../ui/WebViewWindow.h"
#include <string>

class VideoWallpaper : public WallpaperBase {
private:
    HWND m_hwndParent = nullptr;
    WebViewWindow m_webViewManager;
    std::string m_rutaVideo;
    bool m_inicializado = false;

public:
    bool inicializar(HWND hwndParent, const std::string& rutaArchivo) override;
    void actualizar() override;
    void pausar() override;
    void reanudar() override;
    void liberar() override;
};