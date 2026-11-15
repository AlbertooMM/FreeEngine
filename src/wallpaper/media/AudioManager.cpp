#include "AudioManager.h"
#include "WallpaperRenderer.h"
#include <iostream>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

namespace AudioManager {
    static float g_volumenActual = 1.0f; // Valor por defecto

    // Función interna para obtener la ruta del archivo de configuración
    fs::path obtenerRutaConfig() {
        wchar_t buffer[MAX_PATH];
        GetModuleFileNameW(nullptr, buffer, MAX_PATH);
        return fs::path(buffer).parent_path() / "config.json";
    }

    void cargarVolumenGuardado() {
        fs::path ruta = obtenerRutaConfig();
        if (fs::exists(ruta)) {
            std::ifstream archivo(ruta);
            if (archivo.is_open()) {
                std::string linea;
                while (std::getline(archivo, linea)) {
                    // Buscamos algo sencillo como "volumen": 0.75
                    size_t pos = linea.find("\"volumen\":");
                    if (pos != std::string::npos) {
                        try {
                            std::string valStr = linea.substr(pos + 10);
                            g_volumenActual = std::stof(valStr);
                            std::cout << "[AudioManager] Volumen cargado desde disco: " << (g_volumenActual * 100) << "%" << std::endl;
                        } catch (...) {}
                    }
                }
            }
        }
    }

    void guardarVolumenEnDisco(float volumen) {
        fs::path ruta = obtenerRutaConfig();
        // Guardado simple en formato JSON
        std::ofstream archivo(ruta);
        if (archivo.is_open()) {
            archivo << "{\n  \"volumen\": " << volumen << "\n}\n";
        }
    }

    void reproducirAudio(const std::wstring& rutaArchivo) {
        // Gestionado por el fondo activo
    }

    void ajustarVolumen(float volumen) {
        g_volumenActual = volumen;
        std::cout << "[AudioManager] Volumen global establecido a: " << (volumen * 100) << "%" << std::endl;
        
        // Guardamos el cambio en disco
        guardarVolumenEnDisco(volumen);
        
        // Aplicamos al fondo activo
        WallpaperRenderer::actualizarVolumenActivo(volumen);
    }

    float obtenerVolumen() {
        // Si no se ha leído, intentamos cargarlo la primera vez
        static bool cargado = false;
        if (!cargado) {
            cargarVolumenGuardado();
            cargado = true;
        }
        return g_volumenActual;
    }

    void detenerAudio() {
        // Se detiene al cambiar de fondo
    }
}