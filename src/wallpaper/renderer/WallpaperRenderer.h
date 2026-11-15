#pragma once
#include <windows.h>

class WallpaperRenderer {
public:
    // Lee activo.json y lanza la ventana de fondo en el escritorio
    static void iniciarFondoEscritorio();
    static void actualizarFondoActivo();
};