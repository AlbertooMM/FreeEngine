#include <iostream>
#include <filesystem>
#include <string>
#include <windows.h>
#include "Config.h"
#include "SetupDialog.h" // <--- Sustituido por nuestra nueva ventana estilo instalador
#include "WallpaperManager.h"
#include "ActiveWallpaper.h"
#include "WebViewManager.h"
#include "WallpaperRenderer.h"

namespace fs = std::filesystem;

int main()
{
    system("chcp 65001 >nul");

    std::cout << "=== FreeEngine v1.0 - Motor Nativo ===" << std::endl;

    std::string rutaUsuario = Config::leerRutaGuardada();

    if (rutaUsuario.empty() || !fs::exists(rutaUsuario))
    {
        if (!rutaUsuario.empty())
        {
            std::cout << "\n[Aviso] La ruta guardada previamente ya no es válida." << std::endl;
        }
        
        std::cout << "\n[Motor] Abriendo asistente de configuración nativo..." << std::endl;
        
        // Lanza la ventana nativa estilo instalador idéntica a tu boceto
        rutaUsuario = SetupDialog::mostrarVentanaConfiguracion();

        if (rutaUsuario.empty())
        {
            rutaUsuario = "./assets";
            std::cout << "[Info] Ventana cerrada o cancelada. Usando por defecto: './assets'" << std::endl;
        }

        Config::guardarRutaConfig(rutaUsuario);
        std::cout << "[Info] Ruta guardada en 'config.json' para futuras ejecuciones." << std::endl;
    }
    else
    {
        std::cout << "\n[Info] Usando ruta guardada automáticamente: " << rutaUsuario << std::endl;
    }

    // 1. Escanea carpetas y genera el JSON para la interfaz web
    WallpaperManager::escanearYExportarFondos(rutaUsuario);

    // 1.5. Cargar y crear el fondo de escritorio en Windows
    std::cout << "\n[Motor] Cargando fondo de escritorio activo..." << std::endl;
    WallpaperRenderer::iniciarFondoEscritorio();

    // 2. Ruta robusta para encontrar 'index.html' dentro de la carpeta 'menu' junto al .exe
    char buffer[MAX_PATH];
    GetModuleFileNameA(nullptr, buffer, MAX_PATH);
    fs::path dirEjecutable = fs::path(buffer).parent_path();
    
    // Apunta directamente a build/Debug/menu/index.html
    fs::path rutaHtml = dirEjecutable / "menu" / "index.html";

    std::cout << "[Debug] Buscando index.html en: " << rutaHtml.string() << std::endl;
    std::cout << "[Debug] ¿Existe el archivo realmente?: " << (fs::exists(rutaHtml) ? "SÍ" : "NO") << std::endl;

    // 3. Lanza la ventana nativa del Panel de Control con WebView2
    std::cout << "\n[Motor] Abriendo panel de control de FreeEngine...\n" << std::endl;

    WebViewManager::iniciarVentana("FreeEngine - Wallpaper Manager", rutaHtml.string());

    return 0;
}