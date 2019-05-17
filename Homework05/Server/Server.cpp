// Server.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "winsock2.h"
#include "windows.h"
#include "stdio.h"
#include "conio.h"

#define WM_SOCKET WM_USER + 1
#define SERVER_PORT 5500
#define MAX_CLIENT 1024
#define BUFF_SIZE 2048

#pragma comment (lib, "Ws2_32.lib")
#pragma warning(disable : 4996)

typedef struct account
{
	char user[15];
	char pw[15];
	int stt;
	int wrong;
	struct account *next = NULL;
} account;

typedef struct CODE
{
	char LOGIN = '1';
	char LOGOUT = '2';
	char MESSAGE = '3';
	char LOGGED = '1';
	char GUEST = '2';
} CODE;

char *Process(char *buff, int *stt, account *acc);
char *Login(char *uspw, account *acc);
void UpdateAccount(account *acc);
account *LoadAccount();
int Send(SOCKET s, char *buf, int len, int flags);
int Recv(SOCKET s, char *buf, int len, int flags);

// Forward declarations of functions included in this code module:
ATOM MyRegisterClass(HINSTANCE hInstance);
HWND InitInstance(HINSTANCE, int);
LRESULT CALLBACK windowProc(HWND, UINT, WPARAM, LPARAM);

SOCKET client[MAX_CLIENT];
SOCKET listenSock;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{

	MSG msg;
	HWND serverWindow;

	//Registering the Window Class
	MyRegisterClass(hInstance);

	//Create the window
	if ((serverWindow = InitInstance(hInstance, nCmdShow)) == NULL)
		return FALSE;

	//Initiate WinSock
	WSADATA wsaData;
	WORD wVersion = MAKEWORD(2, 2);
	if (WSAStartup(wVersion, &wsaData)) {
		MessageBox(serverWindow, L"Cannot listen!", L"Error!", MB_OK);
		return 0;
	}

	//Construct socket	
	listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	//requests Windows message-based notification of network events for listenSock
	WSAAsyncSelect(listenSock, serverWindow, WM_SOCKET, FD_ACCEPT | FD_CLOSE | FD_READ);

	//Bind address to socket
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(SERVER_PORT);
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(listenSock, (sockaddr *)&serverAddr, sizeof(serverAddr))) {
		MessageBox(serverWindow, L"Cannot bind!", L"Error!", MB_OK);
	}


	//Listen request from client
	if (listen(listenSock, MAX_CLIENT)) {
		MessageBox(serverWindow, L"Cannot listen!", L"Error!", MB_OK);
		return 0;
	}

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}


//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = windowProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = L"WindowClass";
	wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_WINLOGO);

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
HWND InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	HWND hWnd;
	int i;
	for (i = 0; i <MAX_CLIENT; i++)
		client[i] = 0;
	hWnd = CreateWindow(L"WindowClass", L"WSAAsyncSelect TCP Server", WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

	if (!hWnd)
		return FALSE;

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return hWnd;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_SOCKET	- process the events on the sockets
//  WM_DESTROY	- post a quit message and return
//
//

int stt[MAX_CLIENT] = { 0 };
CODE code;
account *acc[MAX_CLIENT];

LRESULT CALLBACK windowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	SOCKET connSock;
	sockaddr_in clientAddr;
	int ret, clientAddrLen = sizeof(clientAddr), i;
	char rcvBuff[BUFF_SIZE], sendBuff[BUFF_SIZE];

	switch (message) {
	case WM_SOCKET:
	{
		if (WSAGETSELECTERROR(lParam)) {
			for (i = 0; i < MAX_CLIENT; i++)
				if (client[i] == (SOCKET)wParam) {
					closesocket(client[i]);
					client[i] = 0;
					continue;
				}
		}

		switch (WSAGETSELECTEVENT(lParam)) {
		case FD_ACCEPT:
		{
			connSock = accept((SOCKET)wParam, (sockaddr *)&clientAddr, &clientAddrLen);
			if (connSock == INVALID_SOCKET) {
				break;
			}
			for (i = 0; i < MAX_CLIENT; i++)
				if (client[i] == 0) {
					client[i] = connSock;
					stt[i] = code.GUEST;
					acc[i] = LoadAccount();
					break;
					//requests Windows message-based notification of network events for listenSock
					WSAAsyncSelect(client[i], hWnd, WM_SOCKET, FD_READ | FD_CLOSE);
				}
			if (i == MAX_CLIENT)
				MessageBox(hWnd, L"Too many clients!", L"Notice", MB_OK);
		}
		break;

		case FD_READ:
		{
			for (i = 0; i < MAX_CLIENT; i++)
				if (client[i] == (SOCKET)wParam)
					break;

			ret = Recv(client[i], rcvBuff, BUFF_SIZE, 0);
			if (ret > 0) {
				rcvBuff[ret] = '\0';
				//echo to client
				//memcpy(sendBuff, rcvBuff, ret);
				char *out = Process(rcvBuff, &stt[i], acc[i]);
				Send(client[i], out, strlen(out), 0);
			}
		}
		break;

		case FD_CLOSE:
		{
			for (i = 0; i < MAX_CLIENT; i++)
				if (client[i] == (SOCKET)wParam) {
					closesocket(client[i]);
					client[i] = 0;
					break;
				}
		}
		break;
		}
	}
	break;

	case WM_DESTROY:
	{
		PostQuitMessage(0);
		shutdown(listenSock, SD_BOTH);
		closesocket(listenSock);
		WSACleanup();
		return 0;
	}
	break;

	case WM_CLOSE:
	{
		DestroyWindow(hWnd);
		shutdown(listenSock, SD_BOTH);
		closesocket(listenSock);
		WSACleanup();
		return 0;
	}
	break;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}


account *LoadAccount()
{
	FILE *f = fopen("account.txt", "r");
	if (f == NULL) {
		printf("Cannot open file!\n");
		system("pause");
		exit(1);
	}
	account *head = NULL, *p1 = (account *)malloc(sizeof(account));
	head = p1;
	char s1[15] = "", s2[15] = "";
	int stt = 2;
	while (1) {
		stt = 2;
		fscanf(f, "%s %s %d\n", s1, s2, &stt);
		if (stt == 2) break;
		account *a = (account *)malloc(sizeof(account));
		strcpy(a->user, s1);
		strcpy(a->pw, s2);
		a->stt = stt;
		a->next = NULL;
		a->wrong = 0;
		p1->next = a;
		p1 = a;
	}
	fclose(f);
	return head;
}

void UpdateAccount(account *acc)
{
	FILE *f = fopen("account.txt", "w+");
	for (account *p = acc->next; p != NULL; p = p->next) {
		fprintf(f, "%s %s %d\n", p->user, p->pw, p->stt);
	}
	fclose(f);
}

char *Process(char *buff, int *pstt, account *acc)
{
	char *out = (char *)malloc(BUFF_SIZE);
	CODE c;
	int code = buff[0];
	if (code == c.LOGIN) {
		if (*pstt == c.LOGGED) {
			char s[] = "You were logged!";
			strcpy(out, s);
		}
		else { // neu la GUEST thi login
			out = Login(buff + 1, acc);
			if (strcmp(out, "Dang nhap thanh cong") == 0) {
				*pstt = c.LOGGED;
			}
		}
	}
	else if (code == c.LOGOUT) {
		if (*pstt == c.GUEST) {
			char s[] = "You are Guest! Cannot Logout!";
			strcpy(out, s);
		}
		else { // dang xuat
			*pstt = c.GUEST;
			char s[] = "Logout done!";
			strcpy(out, s);
		}
	}
	else {
		strcpy(out, buff + 1);
	}
	return out;
}

char *Login(char *uspw, account *acc)
{
	char user[20], pw[20];
	char *out = (char *)malloc(100);
	int i = 0;
	for (i; i < strlen(uspw); i++) {
		if (uspw[i] == ' ')
			break;
	}
	if (i == strlen(uspw)) {
		strcpy(out, "Sai cu phap dang nhap!");
		return out;
	}
	memcpy(user, uspw, i);
	user[i] = '\0';
	memcpy(pw, uspw + i + 1, strlen(uspw) - i);
	pw[strlen(uspw) - i - 1] = '\0';

	strcpy(out, "Sai tai khoan!");

	for (account *p = acc->next; p != NULL; p = p->next) {
		if (strcmp(user, p->user) == 0) {
			if (p->stt == 0) {
				strcpy(out, "Tai khoan da bi khoa!");
			}
			else {
				if (strcmp(pw, p->pw) == 0) {
					p->wrong = 0;
					strcpy(out, "Dang nhap thanh cong");
				}
				else {
					(p->wrong)++;
					strcpy(out, "Sai mat khau!");
					if (p->wrong == 3) {
						p->stt = 0;
						UpdateAccount(acc);
						strcat(out, " Khoa tai khoan ");
						strcat(out, p->user);
					}
				}
			}
		}
	}

	return out;
}

int Send(SOCKET s, char *buf, int len, int flags)
{
	char login[] = "!LOGIN", logout[] = "!LOGOUT";
	CODE c;
	char *sendbuf = (char *)malloc(BUFF_SIZE);
	if (memcmp(buf, login, strlen(login)) == 0) {
		sendbuf[0] = c.LOGIN;
		strcpy(sendbuf + 1, buf + 7);
		len = 1;
	}
	else if (memcmp(buf, logout, strlen(logout)) == 0) {
		sendbuf[0] = c.LOGOUT;
		sendbuf[1] = '\0';
		len = 1;
	}
	else {
		sendbuf[0] = c.MESSAGE;
		strcpy(sendbuf + 1, buf);
		len++;
	}

	char size[4];
	int index = 0, ret;

	size[0] = len / (128 * 128 * 128);
	size[1] = (len - size[0] * 128 * 128 * 128) / (128 * 128);
	size[2] = (len - size[0] * 128 * 128 * 128 - size[1] * 128 * 128) / 128;
	size[3] = len - size[0] * 128 * 128 * 128 - size[1] * 128 * 128 - size[2] * 128;


	// gửi đi kích thước gói tin
	ret = send(s, size, 4, 0);
	if (ret == SOCKET_ERROR) {
		return SOCKET_ERROR;
	}

	while (len > 0) {
		ret = send(s, &sendbuf[index], len, flags);
		if (ret == SOCKET_ERROR) {
			return SOCKET_ERROR;
		}
		len -= ret;
		index += ret;
	}
	return index;
}

int Recv(SOCKET s, char *buf, int len, int flags)
{
	char size[4];
	int index = 0, ret;

	ret = recv(s, size, 4, flags);
	if (ret == SOCKET_ERROR) {
		return SOCKET_ERROR;
	}
	len = size[0] * 128 * 128 * 128 + size[1] * 128 * 128 + size[2] * 128 + size[3];

	while (len > 0) {
		ret = recv(s, &buf[index], len, flags);
		if (ret == SOCKET_ERROR) {
			return SOCKET_ERROR;
		}
		index += ret;
		len -= ret;
	}
	return index;
}
