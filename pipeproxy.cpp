// pipeproxy.cpp : このファイルには 'main' 関数が含まれています。プログラム実行の開始と終了がそこで行われます。
//

#include <windows.h>
#include <iostream>
#include <stdio.h>
#include <string>
#include <thread>

int wmain(int argc, wchar_t* argv[])
{
	if (argc == 1) {
		return 0;
	}

	const wchar_t* pipeName = argv[1];

	std::wstring pipePath{L"\\\\.\\pipe\\" + std::wstring(pipeName)};
	// 親プロセスとの通信用の名前付きパイプを開く
	HANDLE pipeHandle = CreateFile(pipePath.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);
	if (pipeHandle == INVALID_HANDLE_VALUE) {
		auto err = GetLastError();
		wchar_t msg[256];
		wsprintf(msg, L"fail to open named pipe. err:0x%x", err);

		wprintf(L"%s\n", msg);
		return 1;
	}

	std::thread thIn([pipeHandle]() {

		HANDLE hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
		OVERLAPPED overlapped = {};
		overlapped.hEvent = hEvent;

		for(;;) {
			DWORD read = 0;
			char buff[1024];

			ResetEvent(hEvent);
			BOOL b = ReadFile(pipeHandle, buff, 1024, &read, &overlapped);
			if (b != FALSE) {
				fwrite(buff, 1, read, stdout);
				continue;
			}

			DWORD waitResult = WaitForSingleObject(hEvent, 500);
			if (waitResult == WAIT_TIMEOUT) {
				CancelIo(pipeHandle);
				continue;
			}
			GetOverlappedResult(pipeHandle, &overlapped, &read, TRUE);
			fwrite(buff, 1, read, stdout);
		}
	});
	thIn.detach();

	DWORD write = 0;
	std::string input;
	while (std::getline(std::cin, input)) {
		WriteFile(pipeHandle, input.c_str(), (DWORD)input.size(), &write, nullptr);
	}
	return 0;
}
