#pragma once
#include <string>

namespace AudioManager {
    void reproducirAudio(const std::wstring& rutaArchivo);
    void ajustarVolumen(float volumen); // <-- Debe ser float
    float obtenerVolumen();
    void detenerAudio();
}