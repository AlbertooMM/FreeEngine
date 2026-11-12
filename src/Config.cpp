#include "Config.h"
#include <fstream>
#include <sstream>

std::string Config::leerRutaGuardada() {
    try {
        std::ifstream archivo("config.json");
        if (!archivo.is_open()) return "";

        std::string contenido((std::istreambuf_iterator<char>(archivo)), std::istreambuf_iterator<char>());
        size_t pos = contenido.find("\"ruta\"");
        if (pos == std::string::npos) return "";

        size_t start = contenido.find('"', contenido.find(':', pos));
        size_t end = contenido.find('"', start + 1);
        if (start != std::string::npos && end != std::string::npos) {
            return contenido.substr(start + 1, end - start - 1);
        }
    } catch (...) {}
    return "";
}

void Config::guardarRutaConfig(const std::string& ruta) {
    try {
        std::ofstream archivo("config.json");
        if (archivo.is_open()) {
            archivo << "{\n  \"ruta\": \"" << ruta << "\"\n}\n";
        }
    } catch (...) {}
}