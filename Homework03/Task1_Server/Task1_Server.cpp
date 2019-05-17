// Task1_Server.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>

#pragma comment (lib, "Ws2_32.lib")
#pragma warning(disable : 4996)

#define SIZE 1024

typedef struct account
{
	char user[15];
	char pw[15];
	int stt;
	int wrong;
	struct account *next = NULL;
} account;

int Send(SOCKET s, char *buf, int len, int flags);
int Recv(SOCKET s, char *buf, int len, int flags);
char *Process(char *buff, int *stt, account *acc);
char *Login(char *uspw, account *acc);
void UpdateAccount(account *acc);
account *LoadAccount();
unsigned _stdcall echoThread(void *param);

typedef struct CODE
{
	char LOGIN = '1';
	char LOGOUT = '2';
	char MESSAGE = '3';
	char LOGGED = '1';
	char GUEST = '2';
} CODE;

// sử dụng một linken list để các luồng dùng chung
// dùng để xác định một tài khoản đã bị khóa hay chưa
// nếu ở luồng này bị khóa thì trong các luồng khác cũng vậy
volatile account *globalacc;
CRITICAL_SECTION critical;

int main(int argc, const char *argv[])
{
	//Step 1: Inittiate WinSock
	WSADATA wsaData;
	WORD wVersion = MAKEWORD(2, 2);
	if (WSAStartup(wVersion, &wsaData))
		printf("Version is not supported\n");

	//Step 2: Construct socket
	SOCKET listenSock;
	listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	// Port and address
	int port = 5500;
	char addr[] = "127.0.0.1";
	if (argc >= 2) { // đủ 1 tham số dòng lệnh
		port = atoi(argv[1]);
	}

	//Step 3: Bind address to socket
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);
	inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr.s_addr);
	if (bind(listenSock, (sockaddr *)&serverAddr, sizeof(serverAddr))) {
		printf("Error! Cannot bind this address.");
		system("pause");
		return 0;
	}

	// Step 4: Listen request from client
	if (listen(listenSock, 10)) {
		printf("Error!\n");
		system("pause");
		return 0;
	}
	printf("Task 1: Server started!\n");

	//Step 5: Communicate with client
	sockaddr_in clientAddr;
	char buff[SIZE];
	int clientAddrLen = sizeof(clientAddr);

	globalacc = (account *)malloc(sizeof(account));
	globalacc = LoadAccount();
	InitializeCriticalSection(&critical);
	while (1) {
		SOCKET connSock;
		// accept request
		connSock = accept(listenSock, (sockaddr *)&clientAddr, &clientAddrLen);
		printf("Accept: %d!\n", connSock);
		_beginthreadex(0, 0, echoThread, (void *)connSock, 0, 0);
	}
	DeleteCriticalSection(&critical);

	//Step 5: Close socket
	closesocket(listenSock);

	//Step 6: Terminate Winsock
	WSACleanup();
	system("pause");
	return 0;
}

unsigned _stdcall echoThread(void *param)
{
	char buff[SIZE];
	int ret;
	SOCKET connSock = (SOCKET)param;
	CODE code;
	int stt = code.GUEST;
	account *acc = LoadAccount();
	// process
	while (1) {

		// Receive message
		ret = Recv(connSock, buff, SIZE, 0);

		if (ret == SOCKET_ERROR)
			printf("Error : %d", WSAGetLastError());
		else {
			buff[ret] = '\0';

			// sau khi recv thì có byte đầu tiên là mã code của tin nhắn
			printf("Receive from client: %s\n", buff);

			// nếu nhận được xâu BYE từ client thì đóng kết nối	
			if (strcmp(buff, "3BYE") == 0) {
				printf("Close connect!\n");
				shutdown(connSock, SD_SEND);
				closesocket(connSock);
				break;
			}

			char *out = Process(buff, &stt, acc);

			//Echo to client
			ret = Send(connSock, out, strlen(out), 0);
			if (ret == SOCKET_ERROR)
				printf("Error: %", WSAGetLastError());
		}
	}
	shutdown(connSock, SD_SEND);
	closesocket(connSock);
	return 0;
}

account *LoadAccount()
{
	FILE *f = fopen("account.txt", "r");
	if (f == NULL) {
		printf("Cannot open file!");
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
	char *out = (char *)malloc(SIZE);
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

	int globalstt = 0;
	for (account *p = acc->next, *p1 = globalacc->next; p != NULL; p = p->next, p1 = p1->next) {
		if (strcmp(user, p->user) == 0) {
			if (p1->stt == 0) {
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
						EnterCriticalSection(&critical);
						p1->stt = 0;
						LeaveCriticalSection(&critical);
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
	char *sendbuf = (char *)malloc(SIZE);
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