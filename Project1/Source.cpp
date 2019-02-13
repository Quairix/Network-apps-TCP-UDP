#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"user32.lib")
#include <Winsock2.h>
#include <Windows.h> 
#include <Windowsx.h>
#include <CommCtrl.h> 
#include <tchar.h> 
#include <Psapi.h> 
#include <strsafe.h> 
#include <process.h>
#include <locale>

#define IDC_LIST1       2001
#define IDC_BUTTONSEND	2002
#define IDC_TEXTFIELD	2004
#define IDC_PORT		2005
#define IDC_IPFIELD		2006
#define ID_SETPORT		2007
HWND hDlg = NULL;
HWND hFindDlg = NULL;
HWND hList;

CHAR Buf[16] = "";
SOCKET sock = NULL;

HFONT hFont = NULL;
char tmpStr[256] = "";
unsigned int tmpPos = 0;

sockaddr_in sin1 = { 0 };
sockaddr_in sout = { 0 };

LRESULT CALLBACK MyWindowProc(HWND hWnd, UINT uMsg, WPARAM
	wParam, LPARAM lParam);

BOOL PreTranslateMessage(LPMSG lpMsg);

BOOL OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct);
void OnDestroy(HWND hwnd);
void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
HANDLE hThread = NULL;

unsigned __stdcall ThreadFunc(void *lpParameter);
unsigned int receiver = 0; //ip получателя 
#pragma pack(1)
struct MSG_Packet
{
	unsigned __int8 version; //версия
	unsigned short int lenthMessage; //длина текстового сообщения в байтах
	unsigned short int offsset; //номер фрагмента
	unsigned int receiver; //ip получателя 
	unsigned int sender; //ip отправителя
	char text[8]; //текст сообщения

};
#pragma pack()

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE, LPTSTR
	lpszCmdLine, int nCmdShow)
{
	setlocale(LC_ALL, "Russian");
	WNDCLASSEX wcex = { sizeof(WNDCLASSEX) };
	wcex.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wcex.lpfnWndProc = MyWindowProc;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = TEXT("MyWindowClass");
	wcex.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

	if (0 == RegisterClassEx(&wcex))
	{
		return -1;
	}

	LoadLibrary(TEXT("ComCtl32.dll"));
	LoadLibrary(TEXT("Ws2_32.dll"));

	HWND hWnd = CreateWindowEx(0, TEXT("MyWindowClass"), TEXT("UDP Messanger"), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0,
		700, 500, NULL, NULL, hInstance, NULL);

	if (NULL == hWnd)
	{
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

		if (bRet == -1)
		{

		}
		else if (FALSE == bRet)
		{
			break;
		}
		else if (PreTranslateMessage(&msg) == FALSE)
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int)msg.wParam;
}

LRESULT CALLBACK MyWindowProc(HWND hWnd, UINT uMsg, WPARAM
	wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		HANDLE_MSG(hWnd, WM_CREATE, OnCreate);
		HANDLE_MSG(hWnd, WM_DESTROY, OnDestroy);
		HANDLE_MSG(hWnd, WM_COMMAND, OnCommand);
		break;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

BOOL PreTranslateMessage(LPMSG lpMsg)
{
	return IsDialogMessage(hDlg, lpMsg) ||
		IsDialogMessage(hFindDlg, lpMsg);
}

BOOL OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
{
	WSADATA wsaData;
	int err = WSAStartup(MAKEWORD(2, 2), &wsaData);
	hList = CreateWindowEx(0, TEXT("ListBox"), NULL,
		WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | ES_AUTOVSCROLL | WS_HSCROLL, 10, 40, 650, 360, hwnd, (HMENU)IDC_LIST1, lpCreateStruct->hInstance, NULL);

	HWND hwndCtl = CreateWindowEx(0, TEXT("Edit"), NULL,
		WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL, 10, 400, 500, 50,
		hwnd, (HMENU)IDC_TEXTFIELD, lpCreateStruct->hInstance, NULL);

	CreateWindowEx(0, WC_STATIC, TEXT("<- Ip | Port ->"), WS_CHILD | WS_VISIBLE, 220, 10, 140, 20, hwnd, NULL, lpCreateStruct->hInstance, NULL);

	HWND Edit = CreateWindowEx(0, TEXT("Edit"), NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER, 320, 10, 100, 20, hwnd, (HMENU)IDC_PORT, lpCreateStruct->hInstance, NULL);

	CreateWindowEx(0, TEXT("Button"), TEXT("Set Port"),
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 430, 10, 150, 20,
		hwnd, (HMENU)ID_SETPORT, lpCreateStruct->hInstance, NULL);

	CreateWindowEx(0, TEXT("Button"), TEXT("Send"),
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 515, 400, 150, 50,
		hwnd, (HMENU)IDC_BUTTONSEND, lpCreateStruct->hInstance, NULL);

	HWND hwndCtl2 = CreateWindowEx(0, TEXT("Edit"), NULL,
		WS_CHILD | WS_VISIBLE | WS_BORDER, 10, 10, 200, 20,
		hwnd, (HMENU)IDC_IPFIELD, lpCreateStruct->hInstance, NULL);

	Edit_LimitText(Edit, 5);
	Edit_SetText(Edit, TEXT("80"));
	Edit_LimitText(hwndCtl, 255);
	Edit_LimitText(hwndCtl2, 15);
	hThread = (HANDLE)_beginthreadex(NULL, 0, ThreadFunc, NULL, 0, NULL);
	return TRUE;
}

void OnDestroy(HWND hwnd)
{
	closesocket(sock);
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
		case ID_SETPORT:
		{
			EnableWindow(GetDlgItem(hwnd, ID_SETPORT), FALSE);
			sock = socket(AF_INET, SOCK_DGRAM, NULL); //открытие сокета
			sout.sin_family = AF_INET; // семейство адресов = IPv4

			int port = 0;
			char portText[6];
			GetDlgItemTextA(hwnd, IDC_PORT, portText, _countof(portText));//Получение номера порта
			port = atoi(portText);
			if (port < 1)
				port = 74;
			sout.sin_port = htons(port);

			sout.sin_addr.s_addr = htonl(INADDR_ANY);

			int err = bind(sock, (sockaddr *)&sout, sizeof(sout));
			BOOL optval;
			int optlen = sizeof(optval);
			// определяем значение флага SO_BROADCAST
			err = getsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char *)&optval, &optlen);
			if ((SOCKET_ERROR != err) && (TRUE != optval))
			{
				optval = TRUE;
				optlen = sizeof(optval);
				// изменяем значение флага SO_BROADCAST
				err = setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char *)&optval, optlen);
			} // if
			if (SOCKET_ERROR == err)
			{
				/* Не удалось установить флаг SO_BROADCAST*/
			} // if
		}
		break;
		case IDC_BUTTONSEND:
		{
			sin1.sin_family = AF_INET;
			int port = 0;
			char portText[6];
			GetDlgItemTextA(hwnd, IDC_PORT, portText, _countof(portText));//Получение номера порта
			port = atoi(portText);
			if (port < 1)
				port = 74;
			sin1.sin_port = htons(port);
			GetDlgItemTextA(hwnd, IDC_IPFIELD, Buf, 16);
			sin1.sin_addr.s_addr = inet_addr(Buf);
			receiver = atoi(Buf);
			WSADATA wsaData;
			char nn[256];
			struct hostent *host;
			struct in_addr addr;

			WSAStartup(0x0101, &wsaData);//запись ip отправителя
			gethostname(nn, 255);
			host = gethostbyname(nn);
			int ii = 0;
			while (host->h_addr_list[ii] != 0) {
				addr.s_addr = *(u_long *)host->h_addr_list[ii++];
			}
			WCHAR text[255] = L"";
			WCHAR textt[255] = L"";

			MSG_Packet msg1;
			msg1.sender = addr.S_un.S_addr;//запись адр отправителя
			msg1.receiver = receiver;
			msg1.version = 1;
			memset(msg1.text, NULL, 8);//empty msg1 Text field
			char output[256];//строка char для конвертации из wchar
			GetDlgItemTextA(hwnd, IDC_TEXTFIELD, output, 255);
			WCHAR tmpIpString[16];
			mbstowcs(tmpIpString, Buf, 16);

			mbstowcs(text, output, 255);
			StringCchPrintf(textt, 255, L"To %s:\t%s", tmpIpString, text);
			ListBox_AddString(hList, textt);
			int n = 0;
			int lentext = _countof(output)-1; //длина текста
			msg1.lenthMessage = lentext;//заносим длину в структуру
			int num = 0;
			num = 0;

			for (int i = 0; i < lentext; i += 8)
			{
				char part[8] = "";
				for (int j = 0; j < 8; j++)
				{
					part[j] = output[j + i];
				}
				msg1.offsset = num * 8;
				num++;
				for (int j = 0; j < 8; j++)
				{
					msg1.text[j] = part[j];
				}
				n = sendto(sock, (const char *)&msg1, sizeof(msg1), NULL, (struct sockaddr *)&sin1, sizeof(sin1));//отправка пакета
				ZeroMemory(msg1.text, 8);
			}
			WSACleanup();
		}
		break;
		}
	}

}

unsigned __stdcall ThreadFunc(void *lpParameter)
{
	MSG_Packet msgt = { 0 };

	int len = sizeof(sout);
	char text[255] = "";
	WCHAR text1[255] = L"";
	for (;;)
	{
		int n = recvfrom(sock, (char *)&msgt, sizeof(msgt), NULL, (struct sockaddr*)&sout, &len);
		int error = GetLastError();
		if (SOCKET_ERROR != n) // сообщение успешно получено
		{

			char text1[255] = "";
			UINT64 globoffset = 0;
			UINT64 bittext = 0; //массив представляющий собой максимальное число возможных текстовых сообщений 
			do
			{
				bittext = bittext << 8; // смешает числа влево на 8 битов в двоичном преставлении (если были нули то так они и останутся поскольку дополняет число справа нулями
				if (msgt.text[globoffset] >= 0)
					bittext = (bittext | (msgt.text[globoffset])); // при работе с число смещает биттекст на 8 битов влево
				else
					bittext = (bittext | '\0'); // при работе с число смещает биттекст на 8 битов влево
				//дополняя нулями после этого происходит побитовая операция или в ходе которой код следующего символа в побитовом представлении заносится в свободную область из 8 битов 
				globoffset += 1; //счётчик отслеживающий не наступил ли конец сообщения 

				/* по сути в одном сообщении которое мы принимаем может быть только один бит текст который мы просто преобразуем в строку */
				/* которую соответственно будем постепенно объединять в большую строку*/
			} while (globoffset != 8);
			char result[8];
			for (int i = 0; i < 8; i++)
			{
				result[i] = (char)(bittext >> (64 - (i + 1) * 8));
			}
			int c = 0;
			for (int i = msgt.offsset; i < msgt.offsset + 8; i++)
			{
				tmpStr[i] = result[c];
				c++;
				tmpPos++;
			}

			if (tmpPos >= msgt.lenthMessage) //если получили нужную длину - конвертируем в wchar
			{
				WCHAR output[256];
				tmpStr[msgt.lenthMessage] = '\0';
				mbstowcs(output, tmpStr, 256);

				WCHAR output2[278];
				char output1[278];//строка char для конвертации из wchar
				struct in_addr ip_addr1;
				ip_addr1.s_addr = msgt.sender;
				sprintf(output1, "From %s", inet_ntoa(ip_addr1));//конвертируем и выводим Ip
				mbstowcs(output2, output1, 256);

				wsprintf(output2, L"%s : %s", output2, output);//конвертируем и выводим Ip
				ListBox_AddString(hList, output2);
				memset(tmpStr, NULL, 255);
				tmpPos = 0;
			}
		}
	}
	return(0);
}
