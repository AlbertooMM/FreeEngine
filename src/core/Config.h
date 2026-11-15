#pragma once
#include <string>

class Config {
public:
    static std::string leerRutaGuardada();
    static void guardarRutaConfig(const std::string& ruta);
};