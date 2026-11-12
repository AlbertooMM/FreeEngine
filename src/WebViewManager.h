#pragma once
#include <string>

class WebViewManager {
public:
    // Inicializa la ventana y arranca el bucle gráfico cargando la ruta HTML
    static void iniciarVentana(const std::string& tituloVentana, const std::string& rutaHtml);
};