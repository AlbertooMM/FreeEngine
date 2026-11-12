#pragma once
#include <string>

class SetupDialog {
public:
    // Muestra la ventana nativa de configuración y devuelve la ruta elegida (o vacía si cancela)
    static std::string mostrarVentanaConfiguracion();
};