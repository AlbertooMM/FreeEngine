#pragma once

#include <string>

namespace AudioManager {
    void reproducirAudio(const std::wstring& rutaArchivo);
    void ajustarVolumen(int volumen);
    void detenerAudio();
}