cmake_minimum_required(VERSION 3.14)
project(FreeEngine)


if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()
# Usamos el estándar de C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Descargar automáticamente el paquete oficial NuGet de Microsoft WebView2
include(FetchContent)
FetchContent_Declare(
  Microsoft.Web.WebView2
  URL https://www.nuget.org/api/v2/package/Microsoft.Web.WebView2/1.0.2210.55
)
FetchContent_MakeAvailable(Microsoft.Web.WebView2)

# Creamos el ejecutable de la aplicación con todos sus módulos
add_executable(FreeEngine
    src/main.cpp 
    src/Config.cpp 
    src/Config.h 
    src/WallpaperRenderer.cpp
    src/WallpaperRenderer.h
    src/WallpaperManager.cpp 
    src/WallpaperManager.h
    src/ActiveWallpaper.cpp
    src/ActiveWallpaper.h
    src/WebViewManager.cpp
    src/WebViewManager.h
    src/DesktopManager.cpp
    src/DesktopManager.h
    src/WebViewWindow.h
    src/WebViewWindow.cpp
    src/AudioManager.cpp
    src/SetupDialog.h
    src/SetupDialog.cpp
)

# Indicamos dónde están los archivos de cabecera (.h) incluyendo el WebView2 descargado
# 3. ENLAZAR LAS RUTAS DE INCLUSIÓN
target_include_directories(FreeEngine PRIVATE
    src
    include
    "${CMAKE_CURRENT_BINARY_DIR}/_deps/microsoft.web.webview2-src/build/native/include"
)

add_custom_command(TARGET FreeEngine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/menu"
    "$<TARGET_FILE_DIR:FreeEngine>/menu"
)

# Enlazamos las librerías nativas de Windows necesarias para la ventana y WebView2
# Librerías nativas de Windows + Loader estático de WebView2
target_link_libraries(FreeEngine PRIVATE
    shlwapi
    ole32
    oleaut32
    user32
    "${CMAKE_CURRENT_BINARY_DIR}/_deps/microsoft.web.webview2-src/build/native/x64/WebView2LoaderStatic.lib"
)