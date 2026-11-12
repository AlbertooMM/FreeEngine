#pragma once
#include <windows.h>
#include <vector>

struct InfoMonitor {
    RECT rect;
};

class DesktopManager {
public:
    // Obtiene la lista de monitores conectados en el sistema
    static std::vector<InfoMonitor> obtenerMonitores();

    // Incrusta una ventana de Win32 detrás de los iconos del escritorio (WorkerW)
    static void incrustarEnEscritorio(HWND hwndVentana);

    // Posiciona y redimensiona una ventana en las coordenadas de un monitor específico
    static void ajustarAMonitor(HWND hwndVentana, int monitorIndex);
};