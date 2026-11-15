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

# Creamos el ejecutable de la aplicación con todos sus módulos organizados en carpetas
add_executable(FreeEngine
    # Core general
    src/main.cpp 
    src/core/Config.cpp 
    src/core/Config.h 
    src/core/UpdaterManager.cpp
    src/core/UpdaterManager.h

    # UI / Ventanas / WebView2
    src/ui/SetupDialog.cpp
    src/ui/SetupDialog.h
    src/ui/WebViewManager.cpp
    src/ui/WebViewManager.h
    src/ui/WebViewWindow.cpp
    src/ui/WebViewWindow.h

    # Wallpaper Core
    src/wallpaper/core/WallpaperManager.cpp 
    src/wallpaper/core/WallpaperManager.h
    src/wallpaper/core/ActiveWallpaper.cpp
    src/wallpaper/core/ActiveWallpaper.h

    # Wallpaper Renderer / Desktop
    src/wallpaper/renderer/WallpaperRenderer.cpp
    src/wallpaper/renderer/WallpaperRenderer.h
    src/wallpaper/renderer/DesktopManager.cpp
    src/wallpaper/renderer/DesktopManager.h

    # Wallpaper Media
    src/wallpaper/media/AudioManager.cpp
    src/wallpaper/media/AudioManager.h
    src/wallpaper/renderer/types/VideoWallpaper.cpp
)

# Indicamos dónde están los directorios base para que el compilador resuelva los #include
target_include_directories(FreeEngine PRIVATE
    src
    src/core
    src/ui
    src/wallpaper/core
    src/wallpaper/renderer
    src/wallpaper/media
    include
    "${CMAKE_CURRENT_BINARY_DIR}/_deps/microsoft.web.webview2-src/build/native/include"
)

# Copiar recursos del menú tras la compilación
add_custom_command(TARGET FreeEngine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/menu"
    "$<TARGET_FILE_DIR:FreeEngine>/menu"
)

# Enlazamos las librerías nativas de Windows + Loader estático de WebView2
target_link_libraries(FreeEngine PRIVATE
    shlwapi
    ole32
    oleaut32
    user32
    "${CMAKE_CURRENT_BINARY_DIR}/_deps/microsoft.web.webview2-src/build/native/x64/WebView2LoaderStatic.lib"
)