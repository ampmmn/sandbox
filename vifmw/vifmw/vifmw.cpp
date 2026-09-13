#include "framework.h"
#include <cwctype>
#include <string>
#include <vector>

namespace
{
    constexpr wchar_t VifmFileName[] = L"vifm.exe";
    constexpr wchar_t VifmWindowClass[] = L"CASCADIA_HOSTING_WINDOW_CLASS";
    constexpr wchar_t VifmTitleSuffix[] = L"VIFM";

    struct WindowSearchContext
    {
        HWND window = nullptr;
    };

    bool HasSuffix(const std::wstring& value, const wchar_t* suffix)
    {
        const size_t suffixLength = wcslen(suffix);
        return value.size() >= suffixLength &&
            value.compare(value.size() - suffixLength, suffixLength, suffix) == 0;
    }

    BOOL CALLBACK FindVifmWindowCallback(HWND window, LPARAM parameter)
    {
        auto* context = reinterpret_cast<WindowSearchContext*>(parameter);

        wchar_t className[256];
        if (GetClassNameW(window, className, ARRAYSIZE(className)) == 0 ||
            wcscmp(className, VifmWindowClass) != 0)
        {
            return TRUE;
        }

        const int titleLength = GetWindowTextLengthW(window);
        std::wstring title(static_cast<size_t>(titleLength) + 1, L'\0');
        GetWindowTextW(window, &title[0], static_cast<int>(title.size()));
        title.resize(wcslen(title.c_str()));

        if (HasSuffix(title, VifmTitleSuffix))
        {
            context->window = window;
            return FALSE;
        }

        return TRUE;
    }

    HWND FindVifmWindow()
    {
        WindowSearchContext context;
        EnumWindows(FindVifmWindowCallback, reinterpret_cast<LPARAM>(&context));
        return context.window;
    }

    const wchar_t* GetArgumentPart(const wchar_t* commandLine)
    {
        const wchar_t* current = commandLine;

        if (*current == L'"')
        {
            ++current;
            while (*current != L'\0' && *current != L'"')
            {
                ++current;
            }
            if (*current == L'"')
            {
                ++current;
            }
        }
        else
        {
            while (*current != L'\0' && !iswspace(*current))
            {
                ++current;
            }
        }

        return current;
    }
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                      _In_opt_ HINSTANCE hPrevInstance,
                      _In_ LPWSTR lpCmdLine,
                      _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hInstance);
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

    wchar_t modulePath[MAX_PATH];
    const DWORD modulePathLength = GetModuleFileNameW(nullptr, modulePath, ARRAYSIZE(modulePath));
    if (modulePathLength == 0 || modulePathLength >= ARRAYSIZE(modulePath))
    {
        return 1;
    }

    std::wstring vifmPath(modulePath, modulePathLength);
    const size_t separator = vifmPath.find_last_of(L"\\/");
    if (separator == std::wstring::npos)
    {
        return 1;
    }
    vifmPath.resize(separator + 1);
    vifmPath += VifmFileName;

    const wchar_t* argumentPart = GetArgumentPart(GetCommandLineW());
    std::wstring commandLine = L"\"" + vifmPath + L"\"" + argumentPart;
    std::vector<wchar_t> mutableCommandLine(commandLine.begin(), commandLine.end());
    mutableCommandLine.push_back(L'\0');

    STARTUPINFOW startupInfo{};
    startupInfo.cb = sizeof(startupInfo);
    PROCESS_INFORMATION processInfo{};
    if (!CreateProcessW(
        vifmPath.c_str(),
        mutableCommandLine.data(),
        nullptr,
        nullptr,
        FALSE,
        0,
        nullptr,
        nullptr,
        &startupInfo,
        &processInfo))
    {
        return 2;
    }

    CloseHandle(processInfo.hThread);
    CloseHandle(processInfo.hProcess);

    const ULONGLONG deadline = GetTickCount64() + 1000;
    HWND vifmWindow = nullptr;
    do
    {
        vifmWindow = FindVifmWindow();
        if (vifmWindow != nullptr)
        {
            const DWORD targetThreadId = GetWindowThreadProcessId(vifmWindow, nullptr);
            if (targetThreadId != 0)
            {
                AttachThreadInput(GetCurrentThreadId(), targetThreadId, TRUE);
                LONG_PTR style = GetWindowLongPtr(vifmWindow, GWL_STYLE);
                if (style & WS_MINIMIZE) {
                    // ç≈è¨âªÇ≥ÇÍÇƒÇ¢ÇΩÇÁå≥Ç…ñﬂÇ∑
                    PostMessage(vifmWindow, WM_SYSCOMMAND, SC_RESTORE, 0);
                }

                SetForegroundWindow(vifmWindow);
                AttachThreadInput(GetCurrentThreadId(), targetThreadId, FALSE);
            }
            break;
        }

        Sleep(10);
    } while (GetTickCount64() < deadline);

    return vifmWindow == nullptr ? 3 : 0;
}
