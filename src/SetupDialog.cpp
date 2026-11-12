#include "SetupDialog.h"
#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>

#pragma comment(lib, "comctl32.lib")

// Identificadores de controles
#define ID_BTN_BROWSE 101
#define ID_BTN_OK     102
#define ID_BTN_CANCEL 103
#define ID_EDIT_PATH  104

struct DialogData {
    std::wstring rutaSeleccionada;
    HWND hEditPath;
    bool aceptado = false;
};

// Callback de la ventana nativa
LRESULT CALLBACK SetupWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    DialogData* data = reinterpret_cast<DialogData*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    switch (message) {
    case WM_CREATE: {
        CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
        data = reinterpret_cast<DialogData*>(pCreate->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(data));

        // Fuente estándar de Windows (Segoe UI)
        HFONT hFont = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 
                                  OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, 
                                  DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

        // Texto descriptivo superior
        HWND hStatic = CreateWindowW(L"STATIC", L"Selecciona la carpeta en la que deseas tener almacenados tus fondos de pantalla:",
            WS_CHILD | WS_VISIBLE | SS_LEFT, 20, 25, 440, 40, hwnd, NULL, NULL, NULL);
        SendMessage(hStatic, WM_SETFONT, (WPARAM)hFont, TRUE);

        // Cuadro de texto / Botón que muestra la ruta (actúa como el campo del boceto)
        data->hEditPath = CreateWindowW(L"EDIT", data->rutaSeleccionada.c_str(),
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_READONLY, 20, 80, 320, 30, hwnd, (HMENU)ID_EDIT_PATH, NULL, NULL);
        SendMessage(data->hEditPath, WM_SETFONT, (WPARAM)hFont, TRUE);

        // Botón para abrir el explorador de archivos y cambiar la carpeta
        HWND hBtnBrowse = CreateWindowW(L"BUTTON", L"Examinar...",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 350, 80, 110, 30, hwnd, (HMENU)ID_BTN_BROWSE, NULL, NULL);
        SendMessage(hBtnBrowse, WM_SETFONT, (WPARAM)hFont, TRUE);

        // Botón Cancelar
        HWND hBtnCancel = CreateWindowW(L"BUTTON", L"Cancelar",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 260, 145, 90, 32, hwnd, (HMENU)ID_BTN_CANCEL, NULL, NULL);
        SendMessage(hBtnCancel, WM_SETFONT, (WPARAM)hFont, TRUE);

        // Botón Aceptar
        HWND hBtnOk = CreateWindowW(L"BUTTON", L"Aceptar",
            WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 365, 145, 90, 32, hwnd, (HMENU)ID_BTN_OK, NULL, NULL);
        SendMessage(hBtnOk, WM_SETFONT, (WPARAM)hFont, TRUE);
        break;
    }
    case WM_COMMAND: {
        int idCtrl = LOWORD(wParam);
        if (idCtrl == ID_BTN_BROWSE) {
            // Abrir selector nativo moderno de carpetas
            IFileDialog* pFileDialog = nullptr;
            if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFileDialog)))) {
                DWORD dwOptions;
                pFileDialog->GetOptions(&dwOptions);
                pFileDialog->SetOptions(dwOptions | FOS_PICKFOLDERS);
                pFileDialog->SetTitle(L"Seleccionar carpeta de fondos");

                if (SUCCEEDED(pFileDialog->Show(hwnd))) {
                    IShellItem* pItem = nullptr;
                    if (SUCCEEDED(pFileDialog->GetResult(&pItem))) {
                        PWSTR pszPath = nullptr;
                        if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszPath))) {
                            data->rutaSeleccionada = pszPath;
                            SetWindowTextW(data->hEditPath, data->rutaSeleccionada.c_str());
                            CoTaskMemFree(pszPath);
                        }
                        pItem->Release();
                    }
                }
                pFileDialog->Release();
            }
        }
        else if (idCtrl == ID_BTN_OK) {
            data->aceptado = true;
            DestroyWindow(hwnd);
        }
        else if (idCtrl == ID_BTN_CANCEL) {
            data->aceptado = false;
            DestroyWindow(hwnd);
        }
        break;
    }
    case WM_CLOSE:
        data->aceptado = false;
        DestroyWindow(hwnd);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, message, wParam, lParam);
    }
    return 0;
}

std::string SetupDialog::mostrarVentanaConfiguracion() {
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    // Obtener ruta por defecto actual (ej: carpeta assets local)
    wchar_t buffer[MAX_PATH];
    GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    std::wstring defaultPath = std::wstring(buffer).substr(0, std::wstring(buffer).find_last_of(L"\\/")) + L"\\assets";

    DialogData data;
    data.rutaSeleccionada = defaultPath;
    data.aceptado = false;

    HINSTANCE hInstance = GetModuleHandle(nullptr);
    WNDCLASSEXW wc = { sizeof(WNDCLASSEXW) };
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = SetupWndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"FreeEngineSetupClass";
    RegisterClassExW(&wc);

    // Crear ventana centrada en pantalla, tamaño fijo estilo instalador (490x230)
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int posX = (screenWidth - 490) / 2;
    int posY = (screenHeight - 230) / 2;

    HWND hwnd = CreateWindowExW(
        WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
        L"FreeEngineSetupClass",
        L"FreeEngine - Configuración inicial",
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        posX, posY, 490, 230,
        nullptr, nullptr, hInstance, &data
    );

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    // Bucle de mensajes propio para esta ventanita modal
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    CoUninitialize();

    if (data.aceptado) {
        // Convertir wstring a string (UTF-8)
        int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, data.rutaSeleccionada.c_str(), -1, nullptr, 0, nullptr, nullptr);
        std::string str(sizeNeeded, 0);
        WideCharToMultiByte(CP_UTF8, 0, data.rutaSeleccionada.c_str(), -1, &str[0], sizeNeeded, nullptr, nullptr);
        if (!str.empty() && str.back() == '\0') str.pop_back();
        return str;
    }

    return "";
}