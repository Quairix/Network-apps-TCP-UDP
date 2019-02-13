#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include <Winsock2.h>
#include <Windows.h>
#include <windowsx.h>
#include <CommCtrl.h>
#include <tchar.h>
#include <strsafe.h>
#include <process.h>
#pragma comment(lib, "Ws2_32.lib")

#define IDC_STATIC				2001
#define IDC_PORT				2002
#define IDC_EDIT_OPEN_FILE_NAME 2003
#define IDC_BUTTON_CONNECTION	2004
#define IDC_BUTTON_CLOSE		2005
#define IDC_BUTTONOPEN			2006
#define IDC_BUTTONSEND			2007
#define IDC_EDIT1				2011
#define IDC_EDIT2				2012

sockaddr_in sin1 = { 0 };
#pragma pack(1)
struct File_Info
{
	char filename[255];//имя файла
	unsigned int fileSize; //размер файла
};

struct File_Mult
{
	unsigned short filesCount;//количество файлов
	unsigned int filesSumSize; //размер всех файлов для валидации в конце
	char adr[128]; //имя отправителя
};
#pragma pack()

bool scndSend = false;
SOCKET s;
char* folder;//директория файлов
char** fileList;//массив имен файлов
File_Mult Files = { 0 };//структура данных о всех файлах

LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int sendn(SOCKET s, const char *buf, int len);
BOOL OnCreate(HWND hWnd, LPCREATESTRUCT lpCreateStruct);
void OnDestroy(HWND hWnd);
void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE, LPTSTR lpszCmdLine, int nCmdShow) {

	WNDCLASSEX wcex = { sizeof(WNDCLASSEX) };

	wcex.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wcex.lpfnWndProc = WindowProc;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = TEXT("WindowClass");
	wcex.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

	if (0 == RegisterClassEx(&wcex)) {
		return -1;
	}
	LoadLibrary(TEXT("ComCtl32.dll"));
	HWND hWnd = CreateWindowEx(NULL, TEXT("WindowClass"), TEXT("Client"),
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 550, 240, NULL, NULL, hInstance, NULL);

	if (NULL == hWnd) {
		return -1;
	}
	ShowWindow(hWnd, nCmdShow);

	MSG msg;
	BOOL bRet;

	for (;;) {
		while (!PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
		{

		}

		bRet = GetMessage(&msg, NULL, 0, 0);

		if (bRet == -1) {
		}
		else if (FALSE == bRet) {
			break;
		}
		else {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return (int)msg.wParam;
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
		HANDLE_MSG(hWnd, WM_CREATE, OnCreate);
		HANDLE_MSG(hWnd, WM_DESTROY, OnDestroy);
		HANDLE_MSG(hWnd, WM_COMMAND, OnCommand);
		break;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

BOOL OnCreate(HWND hWnd, LPCREATESTRUCT lpCreateStruct) {

	//CreateWindowEx(0, WC_BUTTON, TEXT("Connect to server"),
	//	WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 275, 20, 240, 27, hWnd, (HMENU)IDC_BUTTON_CONNECTION, lpCreateStruct->hInstance, NULL);

	CreateWindowEx(0, TEXT("Edit"), NULL, WS_CHILD | WS_VISIBLE | WS_BORDER, 20, 20, 240, 27, hWnd, (HMENU)IDC_EDIT1, lpCreateStruct->hInstance, NULL);

	CreateWindowEx(0, TEXT("Edit"), NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER, 270, 20, 100, 27, hWnd, (HMENU)IDC_PORT, lpCreateStruct->hInstance, NULL);

	CreateWindowEx(0, WC_STATIC, TEXT("<- IP : Port"), WS_CHILD | WS_VISIBLE, 380, 25, 80, 20, hWnd, NULL, lpCreateStruct->hInstance, NULL);

	CreateWindowEx(0, TEXT("Edit"), NULL, WS_CHILD | WS_VISIBLE | WS_BORDER, 20, 60, 240, 27, hWnd, (HMENU)IDC_EDIT2, lpCreateStruct->hInstance, NULL);

	CreateWindowEx(0, WC_STATIC, TEXT("<- Sender's username"), WS_CHILD | WS_VISIBLE, 275, 60, 240, 27, hWnd, NULL, lpCreateStruct->hInstance, NULL);

	CreateWindowEx(0, WC_BUTTON, TEXT("Close connect"),
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 20, 150, 240, 27, hWnd, (HMENU)IDC_BUTTON_CLOSE, lpCreateStruct->hInstance, NULL);

	CreateWindowEx(0, WC_BUTTON, TEXT("File to send"),
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 20, 110, 240, 27, hWnd, (HMENU)IDC_BUTTONOPEN, lpCreateStruct->hInstance, NULL);

	CreateWindowEx(0, WC_BUTTON, TEXT("Send"),
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 275, 110, 240, 27, hWnd, (HMENU)IDC_BUTTONSEND, lpCreateStruct->hInstance, NULL);

	WSADATA wsaData;
	int err = WSAStartup(MAKEWORD(2, 2), &wsaData);
	s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	return TRUE;
}
void OnDestroy(HWND hWnd)
{
	closesocket(s);
	WSACleanup();
	PostQuitMessage(0);
}

void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
	if (BN_CLICKED == codeNotify)
	{
		HINSTANCE hInstance = GetWindowInstance(hwnd);
		switch (id)
		{
		case IDC_BUTTON_CLOSE:
		{
			int err = shutdown(s, SD_SEND);
		}
		break;
		case IDC_BUTTONOPEN:
		{
			OPENFILENAME ofn;
			char fullFileName[255] = "";
			char fileName[100] = "";
			ZeroMemory(&ofn, sizeof(ofn));
			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = hwnd;
			ofn.hInstance = GetModuleHandle(NULL);
			ofn.lpstrFile = fullFileName;
			ofn.nMaxFile = 150;
			ofn.lpstrFilter = TEXT("");
			ofn.nFilterIndex = 1;
			ofn.lpstrFileTitle = fileName;
			ofn.nMaxFileTitle = 100;
			ofn.lpstrInitialDir = NULL;
			ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_EXPLORER;
			if (GetOpenFileName(&ofn) == TRUE)
			{
				byte fileCount = 0;
				int nOffset = ofn.nFileOffset;
				folder = new char[150];
				StringCchCopy(folder, 150, ofn.lpstrFile);//Имя директории сохранили в глобальную перем

				if (nOffset > lstrlen(ofn.lpstrFile))//более 1 файла
				{
					OPENFILENAME ofn1 = ofn;
					while (ofn.lpstrFile[nOffset])
					{
						nOffset = nOffset + strlen(ofn.lpstrFile + nOffset) + 1;
						fileCount++;
					}

					Files.filesCount = fileCount;//сохранили кол-во файлов
					char** listFiles;
					listFiles = new char*[fileCount];
					nOffset = ofn1.nFileOffset;
					fileCount = 0;

					while (fileCount < Files.filesCount)
					{
						listFiles[fileCount] = new char[100];
						StringCchCopy(listFiles[fileCount++], 100, ofn1.lpstrFile + nOffset);
						nOffset = nOffset + strlen(ofn1.lpstrFile + nOffset) + 1;
					}

					MessageBox(NULL, TEXT("Files selected"), TEXT("Client"), MB_OK | MB_ICONINFORMATION);
					fileList = listFiles; //заносим список файлов в глобальную переменную
				}
				else
				{
					Files.filesCount = 1;
					fileList = new char*[1];
					fileList[0] = new char[100];
					StringCchCopy(fileList[0], 100, ofn.lpstrFile + nOffset);//записали имя единственного файла
					MessageBox(NULL, TEXT("File selected"), TEXT("Client"), MB_OK | MB_ICONINFORMATION);
				}
				scndSend = true;//флаг для очистки folder и filelist
			}
			else
				MessageBox(NULL, TEXT("Error"), TEXT("Client"), MB_OK | MB_ICONINFORMATION);
		}
		break;

		case IDC_BUTTONSEND:
		{
			char bufferNameIP[15];//IP получателя
			GetDlgItemText(hwnd, IDC_EDIT1, bufferNameIP, _countof(bufferNameIP));
			sin1.sin_family = AF_INET;

			int port = 0;
			char portText[6];
			GetDlgItemText(hwnd, IDC_PORT, portText, _countof(portText));//Получение номера порта
			port = atoi(portText);
			if (port < 1)
				port = 74;
			sin1.sin_port = htons(port);

			sin1.sin_addr.s_addr = inet_addr(bufferNameIP);
			int err = connect(s, (sockaddr *)&sin1, sizeof(sin1));
			File_Info *MsgFile = new File_Info[Files.filesCount];//массив данных о файлах
			BYTE **byteBuffer = new BYTE*[Files.filesCount];//двумерный байтовый массив, где хранятся передаваемые файлы

			GetDlgItemTextA(hwnd, IDC_EDIT2, Files.adr, _countof(Files.adr));
			char tmpFileName[255] = "";

			for (int i = 0; i < Files.filesCount; i++)
			{
				_stprintf_s(tmpFileName, 255, _T("%s%s"), folder, fileList[i]);
				HANDLE hFile = CreateFileA(fileList[i], GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
				if (INVALID_HANDLE_VALUE != hFile) {


					LARGE_INTEGER li;
					GetFileSizeEx(hFile, &li);//получили размер файла
					DWORD sizeBuffer = li.LowPart;
					byteBuffer[i] = new BYTE[sizeBuffer];
					Files.filesSumSize += sizeBuffer;//увеличили суммарный размер
					DWORD lpNumberOfBytesRead;
					ReadFile(hFile, byteBuffer[i], sizeBuffer, &lpNumberOfBytesRead, NULL);

					StringCchCopy(MsgFile[i].filename, 255, fileList[i]);//запомнили имя файла
					MsgFile[i].fileSize = sizeBuffer;//Занесли размер файла
					CloseHandle(hFile);
				}
				else
					MessageBox(NULL, TEXT("Error"), TEXT("Client"), MB_OK | MB_ICONINFORMATION);//Ошибка открытия файла
			}
			sendn(s, (const char *)&Files, sizeof(Files));//отправили данные о сумме
			for (int i = 0; i < Files.filesCount; i++)
				sendn(s, (const char *)&MsgFile[i], sizeof(MsgFile[i]));//отправили массив файлов
			for (int i = 0; i < Files.filesCount; i++)
			{
				BYTE* tmpByteStr = byteBuffer[i];
				sendn(s, (const char *)tmpByteStr, MsgFile[i].fileSize);//отправили байтовое содержимое файлов
			}
			for (int i = 0; i < Files.filesCount; i++)
				delete[] byteBuffer[i];
			delete[] byteBuffer;
			delete[] MsgFile;
			if(scndSend)
			{
				delete[] fileList;
				delete[] folder; 
				scndSend = false;
			}
			Files = { 0 };
		}
		break;
		}
	}
}

int sendn(SOCKET s, const char *buf, int len) {
	int size = len;
	while (size > 0)
	{
		int n = send(s, buf, size, 0);
		if (n <= 0)
		{
			if (WSAGetLastError() == WSAEINTR)
			{
				continue;
			}
			return SOCKET_ERROR;
		}
		buf += n;
		size -= n;
	}
	return len;
}

