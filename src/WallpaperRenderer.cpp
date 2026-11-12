#include "WallpaperRenderer.h"
#include "ActiveWallpaper.h"
#include "DesktopManager.h"
#include "WebViewWindow.h"
#include <windows.h>
#include <iostream>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

static HWND g_hwndFondo = nullptr;
static WebViewWindow g_webViewFondoManager;
static UINT_PTR g_timerPausaId = 101;

// Comprueba si hay una ventana maximizada o a pantalla completa en primer plano
bool hayVentanaMaximizadaOEnPantallaCompleta()
{
    HWND hwndForeground = GetForegroundWindow();
    if (!hwndForeground) return false;

    // Si la ventana activa es el propio escritorio o nuestra barra, ignorar
    HWND hwndShell = GetShellWindow();
    if (hwndForeground == hwndShell || hwndForeground == GetDesktopWindow()) 
        return false;

    // Comprobar si la ventana está maximizada
    WINDOWPLACEMENT wp = { sizeof(WINDOWPLACEMENT) };
    if (GetWindowPlacement(hwndForeground, &wp))
    {
        if (wp.showCmd == SW_SHOWMAXIMIZED)
            return true;
    }

    // Comprobar si ocupa toda la pantalla (ej. juegos exclusive fullscreen)
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
        // Configurar un temporizador de 1 segundo para revisar ventanas maximizadas
        SetTimer(hwnd, g_timerPausaId, 1000, nullptr);
        break;
    case WM_TIMER:
        if (wParam == g_timerPausaId)
        {
            static bool estadoPausadoPrevio = false;
            bool debePausar = hayVentanaMaximizadaOEnPantallaCompleta();

            if (debePausar != estadoPausadoPrevio && g_webViewFondoManager.obtenerWebView() != nullptr)
            {
                estadoPausadoPrevio = debePausar;
                if (debePausar)
                {
                    // CORRECCIÓN: ExecuteScript en lugar de ExecuteScriptAsync para C++ nativo
                    g_webViewFondoManager.obtenerWebView()->ExecuteScript(L"if(window.pausarFondo) window.pausarFondo();", nullptr);
                    std::cout << "[WallpaperRenderer] Ventana maximizada detectada. Pausando y silenciando fondo para ahorrar recursos." << std::endl;
                }
                else
                {
                    g_webViewFondoManager.obtenerWebView()->ExecuteScript(L"if(window.reanudarFondo) window.reanudarFondo();", nullptr);
                    std::cout << "[WallpaperRenderer] Escritorio activo. Reanudando fondo y audio." << std::endl;
                }
            }
        }
        break;
    case WM_SIZE:
        {
            RECT bounds;
            GetClientRect(hwnd, &bounds);
            g_webViewFondoManager.redimensionar(bounds);
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

std::string obtenerRutaRecursoFondo(const std::string &idFondo, bool &esVideoOut)
{
    esVideoOut = false;
    if (idFondo.empty())
        return "";

    wchar_t buffer[MAX_PATH];
    GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    fs::path dirEjecutable = fs::path(buffer).parent_path();
    fs::path dirAssets = dirEjecutable / "assets" / idFondo;

    if (!fs::exists(dirAssets))
        return "";

    // Primera pasada: Buscar vídeos (tienen prioridad)
    for (const auto &entry : fs::directory_iterator(dirAssets))
    {
        if (entry.is_regular_file())
        {
            std::string nombreArchivo = entry.path().filename().string();
            std::string ext = entry.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            if (ext == ".mp4" || ext == ".webm" || ext == ".mov" || ext == ".mkv")
            {
                esVideoOut = true;
                return codificarUrl("/assets/" + idFondo + "/" + nombreArchivo);
            }
        }
    }

    // Segunda pasada: Si no hay vídeo, buscar una imagen válida (excluyendo preview)
    for (const auto &entry : fs::directory_iterator(dirAssets))
    {
        if (entry.is_regular_file())
        {
            std::string nombreArchivo = entry.path().filename().string();
            std::string nombreMinuscula = nombreArchivo;
            std::transform(nombreMinuscula.begin(), nombreMinuscula.end(), nombreMinuscula.begin(), ::tolower);

            if (nombreMinuscula.find("preview") != std::string::npos)
                continue;

            std::string ext = entry.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            if (ext == ".jpg" || ext == ".jpeg" || ext == ".png" || ext == ".webp" || ext == ".bmp" || ext == ".jfif")
            {
                esVideoOut = false;
                return codificarUrl("/assets/" + idFondo + "/" + nombreArchivo);
            }
        }
    }

    // Fallback de respaldo
    for (const auto &entry : fs::directory_iterator(dirAssets))
    {
        if (entry.is_regular_file())
        {
            std::string nombreArchivo = entry.path().filename().string();
            std::string ext = entry.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            if (ext == ".jpg" || ext == ".jpeg" || ext == ".png" || ext == ".webp")
            {
                esVideoOut = false;
                return codificarUrl("/assets/" + idFondo + "/" + nombreArchivo);
            }
        }
    }

    return "";
}

void WallpaperRenderer::actualizarFondoActivo()
{
    if (g_webViewFondoManager.obtenerWebView() != nullptr)
    {
        std::string nuevoId = ActiveWallpaper::leerFondoActivo();
        bool esVideo = false;
        std::string nuevaRuta = obtenerRutaRecursoFondo(nuevoId, esVideo);

        if (!nuevaRuta.empty())
        {
            std::string htmlContenido =
                "<html style='margin:0;padding:0;overflow:hidden;background:black;'>"
                "<body style='margin:0;padding:0;width:100vw;height:100vh;overflow:hidden;background:black;display:flex;justify-content:center;align-items:center;'>";

            if (esVideo)
            {
                htmlContenido +=
                    "<video id='wallpaperVideo' src='http://app.engine" + nuevaRuta + "' autoplay loop playsinline style='width:100vw;height:100vh;object-fit:cover;position:absolute;top:0;left:0;'></video>"
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
                    "</script>";
            }
            else
            {
                htmlContenido +=
                    "<img src='http://app.engine" + nuevaRuta + "' style='width:100vw;height:100vh;object-fit:contain;position:absolute;top:0;left:0;'/>";
            }
            htmlContenido += "</body></html>";

            g_webViewFondoManager.navegarATextoHtml(htmlContenido);
            std::cout << "[WallpaperRenderer] Fondo actualizado a ID: " << nuevoId << std::endl;
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

    wchar_t buffer[MAX_PATH];
    GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    fs::path dirEjecutable = fs::path(buffer).parent_path();
    fs::path rutaPerfil = dirEjecutable / "WallpaperUserData";

    bool exitoInit = g_webViewFondoManager.inicializar(
        g_hwndFondo, 
        rutaPerfil.wstring(), 
        L"app.engine", 
        dirEjecutable.wstring()
    );

    if (exitoInit)
    {
        bool esVideo = false;
        std::string rutaRelativa = obtenerRutaRecursoFondo(idFondo, esVideo);

        if (!rutaRelativa.empty())
        {
            std::string htmlContenido =
                "<html style='margin:0;padding:0;overflow:hidden;background:black;'>"
                "<body style='margin:0;padding:0;width:100vw;height:100vh;overflow:hidden;background:black;display:flex;justify-content:center;align-items:center;'>";

            if (esVideo)
            {
                htmlContenido +=
                    "<video id='wallpaperVideo' src='http://app.engine" + rutaRelativa + "' autoplay loop playsinline style='width:100vw;height:100vh;object-fit:cover;position:absolute;top:0;left:0;'></video>"
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
                    "</script>";
            }
            else
            {
                // CORRECCIÓN: Usando 'rutaRelativa' en lugar de 'nuevaRuta'
                htmlContenido +=
                    "<img src='http://app.engine" + rutaRelativa + "' style='width:100vw;height:100vh;object-fit:contain;position:absolute;top:0;left:0;'/>";
            }

            htmlContenido += "</body></html>";

            g_webViewFondoManager.navegarATextoHtml(htmlContenido);
        }
    }
}