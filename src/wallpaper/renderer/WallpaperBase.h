#pragma once
#include <windows.h>
#include <string>

class WallpaperBase {
public:
    virtual ~WallpaperBase() = default;

    // Inicializa el fondo pasándole la ventana de renderizado y la ruta del archivo multimedia
    virtual bool inicializar(HWND hwndParent, const std::string& rutaArchivo) = 0;

    // Actualiza o dibuja el fotograma actual (llamado en el bucle principal)
    virtual void actualizar() = 0;

    // Control de reproducción / estado
    virtual void pausar() = 0;
    virtual void reanudar() = 0;

    // Limpieza de recursos al cerrar o cambiar de fondo
    virtual void liberar() = 0;
};