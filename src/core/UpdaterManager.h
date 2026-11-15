#pragma once

#include <string>

struct InfoActualizacion {
    bool hayNuevaVersio = false;
    std::string ultimaVersion = "";
    std::string urlDescarga = "";
};

class UpdaterManager {
public:
    // Comprueba de forma síncrona (ideal para llamar desde un hilo secundario)
    static InfoActualizacion comprobarActualizaciones(const std::wstring& usuarioRepo, const std::string& versionActual);
};