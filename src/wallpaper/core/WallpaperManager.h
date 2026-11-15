#pragma once
#include <string>
#include <vector>
#include <filesystem>

struct Fondo {
    std::string id;
    std::string titulo;
    std::string video;
    std::string preview;
};

class WallpaperManager {
public:
    static void escanearYExportarFondos(const std::string& rutaDirectorio);

private:
    static std::string rutaAStringSegura(const std::filesystem::path& ruta);
    static std::string extraerValorJson(const std::filesystem::path& rutaJson, const std::string& clave);
    static void guardarJsonFrontend(const std::vector<Fondo>& fondos, const std::string& nombreArchivo);
};