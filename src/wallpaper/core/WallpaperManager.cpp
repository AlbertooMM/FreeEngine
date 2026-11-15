#include <windows.h>
#include "WallpaperManager.h"
#include <iostream>
#include <fstream>

namespace fs = std::filesystem;

std::string WallpaperManager::rutaAStringSegura(const fs::path &ruta)
{
    try
    {
        return ruta.string();
    }
    catch (...)
    {
        try
        {
            return ruta.generic_string();
        }
        catch (...)
        {
            return "[Nombre no compatible]";
        }
    }
}

std::string WallpaperManager::extraerValorJson(const fs::path &rutaJson, const std::string &clave)
{
    try
    {
        std::ifstream archivo(rutaJson);
        if (!archivo.is_open())
            return "";

        std::string linea;
        std::string buscarClave = "\"" + clave + "\"";

        while (std::getline(archivo, linea))
        {
            size_t posClave = linea.find(buscarClave);
            if (posClave != std::string::npos)
            {
                size_t posDosPuntos = linea.find(':', posClave);
                if (posDosPuntos != std::string::npos)
                {
                    size_t posComillaInicio = linea.find('"', posDosPuntos);
                    if (posComillaInicio != std::string::npos)
                    {
                        size_t posComillaFin = linea.find('"', posComillaInicio + 1);
                        if (posComillaFin != std::string::npos)
                        {
                            return linea.substr(posComillaInicio + 1, posComillaFin - posComillaInicio - 1);
                        }
                    }
                }
            }
        }
    }
    catch (...)
    {
    }
    return "Desconocido";
}

void WallpaperManager::guardarJsonFrontend(const std::vector<Fondo> &fondos, const std::string &nombreArchivo)
{
    std::ofstream archivo(nombreArchivo);
    if (!archivo.is_open())
        return;

    archivo << "[\n";
    for (size_t i = 0; i < fondos.size(); ++i)
    {
        archivo << "  {\n";
        archivo << "    \"id\": \"" << fondos[i].id << "\",\n";
        archivo << "    \"titulo\": \"" << fondos[i].titulo << "\",\n";
        archivo << "    \"video\": \"" << fondos[i].video << "\",\n";
        archivo << "    \"preview\": \"" << fondos[i].preview << "\"\n";
        archivo << "  }" << (i < fondos.size() - 1 ? "," : "") << "\n";
    }
    archivo << "]\n";
    archivo.close();
}

void WallpaperManager::escanearYExportarFondos(const std::string &rutaDirectorio)
{
    try
    {
        if (!fs::exists(rutaDirectorio) || !fs::is_directory(rutaDirectorio))
        {
            std::cout << "\n[Error] La ruta configurada no existe o no es un directorio válido: " << rutaDirectorio << std::endl;
            std::cout << "Por favor, borra el archivo 'config.json' para volver a introducir la ruta." << std::endl;
            return;
        }

        std::cout << "\n--- Escaneando y sincronizando desde: " << rutaDirectorio << " ---\n" << std::endl;

        // 1. Obtener la ruta real del ejecutable (build/Debug/) de forma segura
        wchar_t buffer[MAX_PATH];
        GetModuleFileNameW(nullptr, buffer, MAX_PATH);
        fs::path dirEjecutable = fs::path(buffer).parent_path();

        // 2. Forzar que la carpeta assets se cree siempre dentro de build/Debug/assets/
        fs::path rutaAssetsLocal = dirEjecutable / "assets";
        fs::create_directories(rutaAssetsLocal);

        std::vector<Fondo> listaFondos;

        for (const auto &entry : fs::directory_iterator(rutaDirectorio, fs::directory_options::skip_permission_denied))
        {
            try
            {
                if (entry.is_directory())
                {
                    auto path = entry.path();
                    std::string nombreCarpeta = rutaAStringSegura(path.filename());

                    fs::path rutaJson = path / "project.json";
                    bool esProyectoValido = fs::exists(rutaJson);

                    if (esProyectoValido)
                    {
                        // Usar la ruta absoluta del ejecutable para los destinos
                        fs::path destinoFondoPath = rutaAssetsLocal / nombreCarpeta;
                        fs::create_directories(destinoFondoPath);

                        std::string videoEncontrado = "";
                        std::string previewEstatica = "";
                        std::string previewVideo = "";
                        std::string previewGif = "";

                        for (const auto &subEntry : fs::directory_iterator(path, fs::directory_options::skip_permission_denied))
                        {
                            try
                            {
                                if (subEntry.is_regular_file())
                                {
                                    std::string nombreArchivo = rutaAStringSegura(subEntry.path().filename());
                                    std::string ext = rutaAStringSegura(subEntry.path().extension());

                                    if (ext == ".mp4" || ext == ".webm" || ext == ".mov" || ext == ".mkv" ||
                                        ext == ".gif" ||
                                        nombreArchivo == "preview.jpg" || nombreArchivo == "preview.png" || nombreArchivo == "project.json")
                                    {
                                        fs::path rutaOrigen = subEntry.path();
                                        fs::path rutaDestino = destinoFondoPath / nombreArchivo;

                                        std::error_code ec;
                                        fs::copy(rutaOrigen, rutaDestino, fs::copy_options::overwrite_existing, ec);
                                    }

                                    if (videoEncontrado.empty() && (ext == ".mp4" || ext == ".webm" || ext == ".mov" || ext == ".mkv"))
                                    {
                                        videoEncontrado = nombreArchivo;
                                    }

                                    if (nombreArchivo == "preview.jpg" || nombreArchivo == "preview.png")
                                    {
                                        previewEstatica = "/assets/" + nombreCarpeta + "/" + nombreArchivo;
                                    }

                                    if (previewVideo.empty() && (ext == ".mp4" || ext == ".webm"))
                                    {
                                        previewVideo = "/assets/" + nombreCarpeta + "/" + nombreArchivo;
                                    }

                                    if (previewGif.empty() && ext == ".gif")
                                    {
                                        previewGif = "/assets/" + nombreCarpeta + "/" + nombreArchivo;
                                    }
                                }
                            }
                            catch (...) {}
                        }

                        std::string previewFinal = previewEstatica;
                        if (previewFinal.empty()) previewFinal = previewVideo;
                        if (previewFinal.empty()) previewFinal = previewGif;

                        Fondo nuevoFondo;
                        nuevoFondo.id = nombreCarpeta;
                        nuevoFondo.titulo = extraerValorJson(destinoFondoPath / "project.json", "title");
                        
                        if (!videoEncontrado.empty())
                        {
                            nuevoFondo.video = "/assets/" + nombreCarpeta + "/" + videoEncontrado;
                        }
                        else
                        {
                            nuevoFondo.video = "";
                        }

                        nuevoFondo.preview = previewFinal;

                        listaFondos.push_back(nuevoFondo);
                        std::cout << "[Sincronizado] " << nuevoFondo.titulo << " (ID: " << nuevoFondo.id << ")" << std::endl;
                    }
                }
            }
            catch (...)
            {
                continue;
            }
        }

        // Guardar el JSON dentro de la carpeta menu junto al ejecutable
        fs::path rutaMenuDir = dirEjecutable / "menu";
        fs::create_directories(rutaMenuDir);

        fs::path rutaJsonFinal = rutaMenuDir / "fondos.json";
        guardarJsonFrontend(listaFondos, rutaJsonFinal.string());

        std::cout << "[Sincronización] Catálogo guardado en: " << rutaJsonFinal.string() << std::endl;
        std::cout << "\n¡Sincronización completada! Se han procesado " << listaFondos.size() << " fondos.\n" << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cout << "\n[Error crítico en el escaneo]: " << e.what() << std::endl;
    }
}