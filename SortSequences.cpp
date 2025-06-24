#include <windows.h>
#include <shlobj.h>
#include <filesystem>
#include <regex>
#include <vector>
#include <map>

#pragma comment(lib, "Shell32.lib")
namespace fs = std::filesystem;

HWND hEditInput, hLogOutput;
HINSTANCE hInst;

void Log(const std::wstring& text) {
    int len = GetWindowTextLengthW(hLogOutput);
    SendMessage(hLogOutput, EM_SETSEL, len, len);
    SendMessage(hLogOutput, EM_REPLACESEL, FALSE, (LPARAM)text.c_str());
}

void SortSequences(const std::wstring& folder) {
    std::wregex pattern(LR"((.*?)[_-]?(\d+)(\.[a-zA-Z0-9]+))");
    std::map<std::wstring, std::vector<fs::path>> sequences;

    for (const auto& entry : fs::directory_iterator(folder)) {
        if (!entry.is_regular_file()) continue;
        std::wstring filename = entry.path().filename().wstring();
        std::wsmatch match;
        if (std::regex_match(filename, match, pattern)) {
            sequences[match[1]].push_back(entry.path());
        }
    }

    for (const auto& pair : sequences) {
        const std::wstring& base = pair.first;
        const std::vector<fs::path>& files = pair.second;

        fs::path targetDir = fs::path(folder) / base;
        if (!fs::exists(targetDir)) {
            fs::create_directory(targetDir);
            Log(L"📁 Создана папка: " + base + L"\r\n");
        }
        else {
            Log(L"📂 Добавление в папку: " + base + L"\r\n");
        }

        for (const auto& file : files) {
            fs::path dest = targetDir / file.filename();
            if (!fs::exists(dest)) {
                fs::rename(file, dest);
                Log(L"  ➕ " + file.filename().wstring() + L"\r\n");
            }
            else {
                Log(L"  ⚠ Уже существует: " + file.filename().wstring() + L"\r\n");
            }
        }
    }

    if (sequences.empty()) Log(L"❌ Секвенции не найдены.\r\n");
    else Log(L"✅ Готово!\r\n");
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        hEditInput = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
            10, 10, 390, 25, hwnd, nullptr, hInst, nullptr);

        CreateWindowW(L"BUTTON", L"...", WS_CHILD | WS_VISIBLE,
            410, 10, 30, 25, hwnd, (HMENU)2, hInst, nullptr);

        CreateWindowW(L"BUTTON", L"Сортировать", WS_CHILD | WS_VISIBLE,
            10, 45, 120, 30, hwnd, (HMENU)1, hInst, nullptr);

        hLogOutput = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER |
            ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
            10, 85, 460, 160, hwnd, nullptr, hInst, nullptr);
        break;
    }
    case WM_COMMAND: {
        if (LOWORD(wParam) == 1) {
            WCHAR path[MAX_PATH];
            GetWindowTextW(hEditInput, path, MAX_PATH);

            if (wcslen(path) == 0) {
                MessageBoxW(hwnd, L"Вы не выбрали папку!", L"Ошибка", MB_OK | MB_ICONWARNING);
                return 0;
            }

            Log(L"\r\n🔍 Поиск в: " + std::wstring(path) + L"\r\n");
            SortSequences(path);
        }
        else if (LOWORD(wParam) == 2) {
            BROWSEINFOW bi = { 0 };
            WCHAR path[MAX_PATH];
            bi.lpszTitle = L"Выберите папку:";
            bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
            bi.hwndOwner = hwnd;

            PIDLIST_ABSOLUTE pidl = SHBrowseForFolderW(&bi);
            if (pidl && SHGetPathFromIDListW(pidl, path)) {
                SetWindowTextW(hEditInput, path);
            }
            CoTaskMemFree(pidl);
        }
        break;
    }
    case WM_DESTROY:
        PostQuitMessage(0); return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow) {
    hInst = hInstance;

    const wchar_t CLASS_NAME[] = L"SortSequencesWin";

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(0, CLASS_NAME, L"Сортировка секвенций", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 500, 300,
        nullptr, nullptr, hInstance, nullptr);

    ShowWindow(hwnd, nCmdShow);

    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
