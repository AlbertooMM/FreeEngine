#include "ActiveWallpaper.h"
#include <windows.h>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

static fs::path obtenerRutaActivoJson() {
    wchar_t buffer[MAX_PATH];
    GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    return fs::path(buffer).parent_path() / "activo.json";
}

std::string ActiveWallpaper::leerFondoActivo() {
    try {
        std::ifstream archivo(obtenerRutaActivoJson());
        if (!archivo.is_open()) return "";

        std::string contenido((std::istreambuf_iterator<char>(archivo)), std::istreambuf_iterator<char>());
        size_t pos = contenido.find("\"idActivo\"");
        if (pos == std::string::npos) return "";

        size_t start = contenido.find('"', contenido.find(':', pos));
        size_t end = contenido.find('"', start + 1);
        if (start != std::string::npos && end != std::string::npos) {
            return contenido.substr(start + 1, end - start - 1);
        }
    } catch (...) {}
    return "";
}

int ActiveWallpaper::leerMonitorActivo() {
    try {
        std::ifstream archivo(obtenerRutaActivoJson());
        if (!archivo.is_open()) return 0;

        std::string contenido((std::istreambuf_iterator<char>(archivo)), std::istreambuf_iterator<char>());
        size_t pos = contenido.find("\"monitorIndex\"");
        if (pos == std::string::npos) return 0;

        size_t colon = contenido.find(':', pos);
        if (colon == std::string::npos) return 0;

        size_t start = colon + 1;
        while (start < contenido.size() && (contenido[start] == ' ' || contenido[start] == '\r' || contenido[start] == '\n')) {
            start++;
        }
        size_t end = start;
        while (end < contenido.size() && std::isdigit(contenido[end])) {
            end++;
        }

        if (start < end) {
            return std::stoi(contenido.substr(start, end - start));
        }
    } catch (...) {}
    return 0;
}

void ActiveWallpaper::guardarFondoActivo(const std::string& idFondo, int monitorIndex) {
    try {
        std::ofstream archivo(obtenerRutaActivoJson());
        if (archivo.is_open()) {
            archivo << "{\n";
            archivo << "  \"idActivo\": \"" << idFondo << "\",\n";
            archivo << "  \"monitorIndex\": " << monitorIndex << "\n";
            archivo << "}\n";
        }
    } catch (...) {}
}