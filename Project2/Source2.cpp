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

#define IDC_BUTTONSTART 2001
#define IDC_BUTTON2 2002
#define IDC_PORT 2003

#pragma pack(1)
struct File_Info
{
	char fileName[255];//полное имя файла
	unsigned int fileSize; //размер файла
};

struct File_Mult
{
	unsigned short filesCount;//количество файлов
	unsigned int filesSumSize; //размер всех файлов для валидации в конце
	char adr[128]; //   отправителя
};

#pragma pack()
SOCKET s;
SOCKET ss;
sockaddr_in sOut;

File_Mult Files;

HANDLE hFile = NULL;
HANDLE hThread = NULL;
HANDLE hThread1 = NULL;

unsigned _stdcall AcceptThread(LPVOID);
int recvn(SOCKET s, char *buf, int len);
unsigned _stdcall ListenThread(void *lpParameter);

LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

BOOL OnCreate(HWND hWnd, LPCREATESTRUCT lpCreateStruct);
void OnDestroy(HWND hWnd);

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

	HWND hWnd = CreateWindowEx(NULL, TEXT("WindowClass"), TEXT("Server"),
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 550, 100, NULL, NULL, hInstance, NULL);

	if (NULL == hWnd) {
		return -1;
	}
	ShowWindow(hWnd, nCmdShow);
	MSG msg;
	BOOL bRet;

	for (;;)
	{
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
	WSACleanup();
	return (int)msg.wParam;
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg)
	{
		HANDLE_MSG(hWnd, WM_CREATE, OnCreate);
		HANDLE_MSG(hWnd, WM_DESTROY, OnDestroy);

	case WM_COMMAND:
	{
		switch (LOWORD(wParam)) {

		case IDC_BUTTON2:
		{
			int err = shutdown(s, SD_BOTH);
		}
		break;
		case IDC_BUTTONSTART:
		{
			WSADATA wsaData;
			int err = WSAStartup(MAKEWORD(2, 2), &wsaData);
			if (0 == err) {
				s = socket(AF_INET, SOCK_STREAM, 0);
				if (s != INVALID_SOCKET)
				{
					int port = 0;
					char portText[6];
					GetDlgItemText(hWnd, IDC_PORT, portText, _countof(portText));//Получение номера порта
					port = atoi(portText);
					if (port < 1)
						port = 74;
					sOut = { AF_INET, htons(port), INADDR_ANY };
					bind(s, (sockaddr *)&sOut, sizeof(sOut));
					int err = listen(s, 5);
					if (SOCKET_ERROR != err)
					{
						hThread1 = (HANDLE)_beginthreadex(NULL, 0, AcceptThread, 0, 0, NULL);
					}
				}
			}
		}
		break;
		return TRUE;
		}
		return 0;
	}
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

BOOL OnCreate(HWND hWnd, LPCREATESTRUCT lpCreateStruct)
{
	HWND Edit = CreateWindowEx(0, TEXT("Edit"), TEXT("74"),
		WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER, 50, 10, 90, 20, hWnd, (HMENU)IDC_PORT, lpCreateStruct->hInstance, NULL);

	CreateWindowEx(0, WC_STATIC, TEXT("Port:"), WS_CHILD | WS_VISIBLE, 10, 10, 40, 20, hWnd, NULL, lpCreateStruct->hInstance, NULL);

	CreateWindowEx(0, WC_BUTTON, TEXT("Start server"),
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 150, 10, 100, 20, hWnd, (HMENU)IDC_BUTTONSTART, lpCreateStruct->hInstance, NULL);

	CreateWindowEx(0, WC_BUTTON, TEXT("Close connection"),
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 260, 10, 130, 20, hWnd, (HMENU)IDC_BUTTON2, lpCreateStruct->hInstance, NULL);

	Edit_LimitText(Edit, 5);
	return TRUE;
}

void OnDestroy(HWND hWnd)
{
	closesocket(s);
	PostQuitMessage(0);
}

int recvn(SOCKET s, char *buf, int len)
{
	int size = len;
	while (size > 0)
	{
		int n = recv(s, buf, size, 0);
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

unsigned _stdcall ListenThread(void *lpParameter)
{
	SOCKET ss = (SOCKET)lpParameter;
	for (;;)
	{
		if (INVALID_SOCKET == ss)
		{
			if (WSAEINTR == WSAGetLastError()) break;
		}
		else
		{
			CHAR name[MAX_PATH] = "";
			recvn(ss, (char *)&Files, sizeof(Files));
			File_Info *msgFile = new File_Info[Files.filesCount];

			for (int i = 0; i < Files.filesCount; i++)
				recvn(ss, (char *)&msgFile[i], sizeof(msgFile[i])); //получаем сколько

			for (int i = 0; i < Files.filesCount; i++) //начало обработки цикла получения сообщений по кол-ву тех что указаны в инфо
			{
				BYTE* tmpByteStr = new BYTE[msgFile[i].fileSize];
				recvn(ss, (char *)tmpByteStr, msgFile[i].fileSize); //получаем файлы в двумерный массив
				hFile = CreateFile(msgFile[i].fileName, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL);
				if (INVALID_HANDLE_VALUE != hFile)
				{
					DWORD nNumberOfBytesToWrite;
					WriteFile(hFile, tmpByteStr, msgFile[i].fileSize, &nNumberOfBytesToWrite, NULL);
				}
				CloseHandle(hFile);
				delete[] tmpByteStr;
			}
			char adr1[255] = "";
			StringCchPrintf(adr1, 39, TEXT("%d File(s) from %s"), Files.filesCount, Files.adr);
			MessageBox(NULL, adr1, TEXT("Server"), MB_OK | MB_ICONINFORMATION);
			delete[] msgFile;
		}
	}
	FindClose(hFile);
	closesocket(ss);
	return 0;
}
unsigned _stdcall AcceptThread(LPVOID)
{
	while(true){
		SOCKET ss = accept(s, NULL, NULL);
		if (INVALID_SOCKET == ss)
		{
			if (WSAEINTR == WSAGetLastError()) break; //прерывание если была ошибка
		}
		else
		{
			hThread = (HANDLE)_beginthreadex(NULL, 0, ListenThread, (SOCKET *)ss, 0, NULL);
			MessageBox(NULL, TEXT("Successfull connect"), TEXT("Server"), MB_OK | MB_ICONINFORMATION);
			CloseHandle(hThread);
		}
	}
	return 0;
}