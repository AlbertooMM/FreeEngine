#include "WallpaperRenderer.h"
#include "ActiveWallpaper.h"
#include "DesktopManager.h"
#include "types/VideoWallpaper.h"
#include <windows.h>
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <memory>

namespace fs = std::filesystem;

static HWND g_hwndFondo = nullptr;
static std::unique_ptr<WallpaperBase> g_wallpaperActual = nullptr;
static UINT_PTR g_timerPausaId = 101;

// Definición de las variables globales compartidas para el WebView unificado
WebViewWindow g_sharedWallpaperWebView;
bool g_sharedWebViewIniciado = false;

// Comprueba si hay una ventana maximizada o a pantalla completa en primer plano
bool hayVentanaMaximizadaOEnPantallaCompleta()
{
    HWND hwndForeground = GetForegroundWindow();
    if (!hwndForeground) return false;

    HWND hwndShell = GetShellWindow();
    if (hwndForeground == hwndShell || hwndForeground == GetDesktopWindow()) 
        return false;

    WINDOWPLACEMENT wp = { sizeof(WINDOWPLACEMENT) };
    if (GetWindowPlacement(hwndForeground, &wp))
    {
        if (wp.showCmd == SW_SHOWMAXIMIZED)
            return true;
    }

    RECT rcWindow, rcScreen;
    GetWindowRect(hwndForeground, &rcWindow);
    rcScreen.left = GetSystemMetrics(SM_XVIRTUALSCREEN);
    rcScreen.top = GetSystemMetrics(SM_YVIRTUALSCREEN);
    rcScreen.right = rcScreen.left + GetSystemMetrics(SM_CXVIRTUALSCREEN);
    rcScreen.bottom = rcScreen.top + GetSystemMetrics(SM_CYVIRTUALSCREEN);

    if (rcWindow.left <= rcScreen.left && rcWindow.top <= rcScreen.top &&
        rcWindow.right >= rcScreen.right && rcWindow.bottom >= rcScreen.bottom)
    {
        return true;
    }

    return false;
}

LRESULT CALLBACK WndProcFondo(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
        SetTimer(hwnd, g_timerPausaId, 1000, nullptr);
        break;
    case WM_TIMER:
        if (wParam == g_timerPausaId)
        {
            static bool estadoPausadoPrevio = false;
            bool debePausar = hayVentanaMaximizadaOEnPantallaCompleta();

            if (debePausar != estadoPausadoPrevio && g_wallpaperActual != nullptr)
            {
                estadoPausadoPrevio = debePausar;
                if (debePausar)
                {
                    g_wallpaperActual->pausar();
                    std::cout << "[WallpaperRenderer] Ventana maximizada detectada. Pausando fondo." << std::endl;
                }
                else
                {
                    g_wallpaperActual->reanudar();
                    std::cout << "[WallpaperRenderer] Escritorio activo. Reanudando fondo." << std::endl;
                }
            }
        }
        break;
    case WM_SIZE:
        if (g_wallpaperActual != nullptr)
        {
            g_wallpaperActual->actualizar();
        }
        break;
    case WM_DESTROY:
        KillTimer(hwnd, g_timerPausaId);
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, message, wParam, lParam);
    }
    return 0;
}

std::string codificarUrl(const std::string &ruta)
{
    std::string resultado = "";
    for (char c : ruta)
    {
        if (c == ' ')
            resultado += "%20";
        else
            resultado += c;
    }
    return resultado;
}

std::string obtenerRutaRecursoFondo(const std::string &idFondo)
{
    if (idFondo.empty())
        return "";

    wchar_t buffer[MAX_PATH];
    GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    fs::path dirEjecutable = fs::path(buffer).parent_path();
    fs::path dirAssets = dirEjecutable / "assets" / idFondo;

    if (!fs::exists(dirAssets))
    {
        std::cout << "[Fondo] ERROR: No existe la carpeta de assets para ID: " << idFondo << std::endl;
        return "";
    }

    // Buscar archivos de vídeo
    for (const auto &entry : fs::directory_iterator(dirAssets))
    {
        if (entry.is_regular_file())
        {
            std::string nombreArchivo = entry.path().filename().string();
            std::string ext = entry.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            if (ext == ".mp4" || ext == ".webm" || ext == ".mov" || ext == ".mkv")
            {
                std::cout << "[Fondo] Vídeo encontrado: " << nombreArchivo << std::endl;
                return codificarUrl("/assets/" + idFondo + "/" + nombreArchivo);
            }
        }
    }

    std::cout << "[Fondo] ADVERTENCIA: No se encontró ningún vídeo válido en la carpeta de ID: " << idFondo << std::endl;
    return "";
}

void WallpaperRenderer::actualizarFondoActivo()
{
    std::string nuevoId = ActiveWallpaper::leerFondoActivo();
    std::string nuevaRuta = obtenerRutaRecursoFondo(nuevoId);

    if (!nuevaRuta.empty() && g_hwndFondo != nullptr)
    {
        // Como de momento solo manejamos vídeo, instanciamos o reutilizamos VideoWallpaper
        if (dynamic_cast<VideoWallpaper*>(g_wallpaperActual.get()) == nullptr) {
            g_wallpaperActual = std::make_unique<VideoWallpaper>();
        }

        if (g_wallpaperActual) {
            g_wallpaperActual->inicializar(g_hwndFondo, nuevaRuta);
            std::cout << "[WallpaperRenderer] Fondo de vídeo actualizado a ID: " << nuevoId << std::endl;
        }
    }
}

void WallpaperRenderer::iniciarFondoEscritorio()
{
    std::string idFondo = ActiveWallpaper::leerFondoActivo();
    int monitorIndex = ActiveWallpaper::leerMonitorActivo();

    std::cout << "[WallpaperRenderer] Iniciando fondo de escritorio...\n";

    HINSTANCE hInstance = GetModuleHandle(nullptr);

    WNDCLASSEXW wc = {sizeof(WNDCLASSEXW)};
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProcFondo;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(BLACK_BRUSH);
    wc.lpszClassName = L"FreeEngineWallpaperClass";
    RegisterClassExW(&wc);

    g_hwndFondo = CreateWindowExW(
        WS_EX_TOOLWINDOW,
        L"FreeEngineWallpaperClass",
        L"FreeEngine - Fondo de Escritorio",
        WS_POPUP | WS_VISIBLE,
        0, 0, 1920, 1080,
        nullptr, nullptr, hInstance, nullptr);

    if (!g_hwndFondo) return;

    DesktopManager::incrustarEnEscritorio(g_hwndFondo);
    DesktopManager::ajustarAMonitor(g_hwndFondo, monitorIndex);

    LONG exStyle = GetWindowLong(g_hwndFondo, GWL_EXSTYLE);
    SetWindowLong(g_hwndFondo, GWL_EXSTYLE, exStyle & ~WS_EX_TRANSPARENT);

    ShowWindow(g_hwndFondo, SW_SHOW);
    SetWindowPos(g_hwndFondo, HWND_BOTTOM, 0, 0, 0, 0, 
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);

    std::string rutaRelativa = obtenerRutaRecursoFondo(idFondo);

    if (!rutaRelativa.empty())
    {
        g_wallpaperActual = std::make_unique<VideoWallpaper>();
        g_wallpaperActual->inicializar(g_hwndFondo, rutaRelativa);
    }
}