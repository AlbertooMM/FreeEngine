#include "DesktopManager.h"
#include <iostream>

BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {
    std::vector<InfoMonitor>* monitores = reinterpret_cast<std::vector<InfoMonitor>*>(dwData);
    monitores->push_back({ *lprcMonitor });
    return TRUE;
}

std::vector<InfoMonitor> DesktopManager::obtenerMonitores() {
    std::vector<InfoMonitor> listaMonitores;
    EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, reinterpret_cast<LPARAM>(&listaMonitores));
    return listaMonitores;
}

void DesktopManager::incrustarEnEscritorio(HWND hwndFondo)
{
    std::cout << "\n========================================\n";
    std::cout << "[DesktopManager] >>> INCRUSTACION DEFINITIVA (v5) <<<\n";
    std::cout << "========================================\n" << std::endl;
    std::cout.flush();

    HWND hwndProgman = FindWindowW(L"Progman", nullptr);
    if (!hwndProgman) return;

    // 1. Enviar el mensaje 0x052C a Progman para forzar la separación del fondo y los iconos
    DWORD_PTR result = 0;
    SendMessageTimeoutW(hwndProgman, 0x052C, 0xD, 0x1, SMTO_NORMAL, 1000, &result);

    HWND hwndWorkerW = nullptr;

    // 2. Bucle para cazar al vuelo el WorkerW recién creado detrás de la vista de iconos (SHELLDLL_DefView)
    for (int i = 0; i < 15; ++i) {
        EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
            HWND hwndShellView = FindWindowExW(hwnd, nullptr, L"SHELLDLL_DefView", nullptr);
            if (hwndShellView != nullptr)
            {
                HWND* pFoundWorkerW = (HWND*)lParam;
                // El WorkerW real es el hermano que está por debajo de la ventana del shell
                *pFoundWorkerW = FindWindowExW(nullptr, hwnd, L"WorkerW", nullptr);
                if (*pFoundWorkerW != nullptr) {
                    return FALSE; // Detener enumeración al encontrarlo
                }
            }
            return TRUE;
        }, (LPARAM)&hwndWorkerW);

        if (hwndWorkerW != nullptr) break;
        Sleep(50); // Pequeña pausa si Windows tarda unos milisegundos en componerlo
    }

    // 3. Transformar estilo de ventana obligatoriamente a WS_CHILD sin bordes
    LONG_PTR estiloActual = GetWindowLongPtr(hwndFondo, GWL_STYLE);
    estiloActual &= ~(WS_POPUP | WS_CAPTION | WS_THICKFRAME);
    estiloActual |= WS_CHILD | WS_VISIBLE;
    SetWindowLongPtr(hwndFondo, GWL_STYLE, estiloActual);

    // 4. Asignar el padre estrictamente al WorkerW real (jamás a Progman directamente)
    if (hwndWorkerW != nullptr)
    {
        std::cout << "[DesktopManager] ¡ÉXITO ROTUNDO! WorkerW real localizado (" << hwndWorkerW << "). Anclando...\n";
        SetParent(hwndFondo, hwndWorkerW);
    }
    else
    {
        // Si por alguna razón extrema de rendimiento no lo hallara, 
        // buscamos el WorkerW principal del propio Progman en lugar del contenedor raíz
        std::cout << "[DesktopManager] AVISO: Buscando WorkerW secundario de Progman...\n";
        HWND hwndWorkerWFallback = FindWindowExW(hwndProgman, nullptr, L"WorkerW", nullptr);
        if (hwndWorkerWFallback) {
            SetParent(hwndFondo, hwndWorkerWFallback);
        } else {
            SetParent(hwndFondo, hwndProgman);
        }
    }

    // 5. Dimensiones totales de pantalla y posicionamiento al fondo absoluto
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);

    SetWindowPos(hwndFondo, HWND_BOTTOM, 0, 0, screenW, screenH, 
        SWP_NOACTIVATE | SWP_SHOWWINDOW);
    
    ShowWindow(hwndFondo, SW_SHOW);
}

void DesktopManager::ajustarAMonitor(HWND hwndVentana, int monitorIndex) {
    auto monitores = obtenerMonitores();
    if (monitores.empty()) return;

    if (monitorIndex < 0 || monitorIndex >= monitores.size()) {
        monitorIndex = 0; 
    }

    RECT r = monitores[monitorIndex].rect;
    int ancho = r.right - r.left;
    int alto = r.bottom - r.top;

    SetWindowPos(hwndVentana, HWND_BOTTOM, r.left, r.top, ancho, alto, SWP_NOACTIVATE | SWP_SHOWWINDOW);
}