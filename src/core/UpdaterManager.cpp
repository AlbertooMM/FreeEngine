#include "UpdaterManager.h"
#include <windows.h>
#include <winhttp.h>
#include <iostream>

#pragma comment(lib, "winhttp.lib")

InfoActualizacion UpdaterManager::comprobarActualizaciones(const std::wstring& usuarioRepo, const std::string& versionActual) {
    InfoActualizacion info;
    
    // Usamos nullptr en lugar de WINHTTP_NO_PROXY_VAL para evitar conflictos de macros
    HINTERNET hSession = WinHttpOpen(L"FreeEngine-Updater/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, nullptr, nullptr, 0);
    if (!hSession) return info;

    HINTERNET hConnect = WinHttpConnect(hSession, L"api.github.com", INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect) {
        WinHttpCloseHandle(hSession);
        return info;
    }

    std::wstring rutaApi = L"/repos/" + usuarioRepo + L"/releases/latest";
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", rutaApi.c_str(), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    
    if (hRequest) {
        LPCWSTR headers = L"User-Agent: FreeEngine-App\r\nAccept: application/vnd.github+json";
        if (WinHttpSendRequest(hRequest, headers, -1L, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
            WinHttpReceiveResponse(hRequest, NULL)) {
            
            DWORD dwSize = 0;
            DWORD dwDownloaded = 0;
            std::string respuesta;

            do {
                dwSize = 0;
                if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) break;
                if (!dwSize) break;

                char* pszOutBuffer = new char[dwSize + 1];
                ZeroMemory(pszOutBuffer, dwSize + 1);

                if (WinHttpReadData(hRequest, (LPVOID)pszOutBuffer, dwSize, &dwDownloaded)) {
                    respuesta.append(pszOutBuffer, dwDownloaded);
                }
                delete[] pszOutBuffer;
            } while (dwSize > 0);

            size_t posTag = respuesta.find("\"tag_name\":\"");
            if (posTag != std::string::npos) {
                size_t inicio = posTag + 12;
                size_t fin = respuesta.find("\"", inicio);
                if (fin != std::string::npos) {
                    info.ultimaVersion = respuesta.substr(inicio, fin - inicio);
                    if (info.ultimaVersion != versionActual) {
                        info.hayNuevaVersio = true;
                    }
                }
            }

            size_t posBrowser = respuesta.find("\"browser_download_url\":\"");
            if (posBrowser != std::string::npos) {
                size_t inicio = posBrowser + 24;
                size_t fin = respuesta.find("\"", inicio);
                if (fin != std::string::npos) {
                    info.urlDescarga = respuesta.substr(inicio, fin - inicio);
                }
            }
        }
        WinHttpCloseHandle(hRequest);
    }
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    return info;
}