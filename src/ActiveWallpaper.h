#pragma once
#include <string>

class ActiveWallpaper {
public:
    static std::string leerFondoActivo();
    static int leerMonitorActivo();
    static void guardarFondoActivo(const std::string& idFondo, int monitorIndex);
};