// Task1_Server.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma comment (lib, "Ws2_32.lib")
// lỗi inet_addr và inet_ntoa: chương trình yêu cầu dùng hàm inet_pton và inet_ntop

#define SIZE 10240

int Send(SOCKET s, char *buf, int len, int flags);
int Recv(SOCKET s, char *buf, int len, int flags);
char *EditBuff(char *buff);
int classify(char x);

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
	char straddr[INET_ADDRSTRLEN];
	char buff[SIZE];
	int ret, clientAddrLen = sizeof(clientAddr);

	while (1) {
		SOCKET connSock;
		// accept request
		connSock = accept(listenSock, (sockaddr *)&clientAddr, &clientAddrLen);
		printf("Accept!\n");

		while (1) {

			// Receive message
			ret = Recv(connSock, buff, SIZE, 0);

			if (ret == SOCKET_ERROR)
				printf("Error : %d", WSAGetLastError());
			else {
				buff[ret] = '\0';

				inet_ntop(AF_INET, &clientAddr.sin_addr, straddr, INET_ADDRSTRLEN);
				printf("Receive from client[%s:%d] %s\n", straddr, ntohs(clientAddr.sin_port), buff);

				// nếu nhận được xâu rỗng từ client thì đóng kết nối
				if (strlen(buff) == 0) {
					printf("Close connect!\n");
					shutdown(connSock, SD_SEND);
					closesocket(connSock);
					break;
				}

				// Xử lý tin nhắn của client
				// Không dùng được regular -> xử lý chay
				char *out = EditBuff(buff);
				//Echo to client
				ret = Send(connSock, out, strlen(out), 0);
				if (ret == SOCKET_ERROR)
					printf("Error: %", WSAGetLastError());
			}
		}
	}

	//Step 5: Close socket
	closesocket(listenSock);

	//Step 6: Terminate Winsock
	WSACleanup();
	system("pause");
	return 0;
}

char *EditBuff(char *buff)
{
	char out[SIZE];
	int len = strlen(buff);
	if (len == 0)
		return buff;
	char error[] = "Error: Input string contains special character";
	for (int i = 0; i < len; i++) {
		// có ký tự ngoài chữ cái và số thì return lỗi luôn
		if (classify(buff[i]) == 0) {
			strcpy_s(out, error);
			return out;
		}
	}
	// loại ký tự vừa duyệt
	int last = classify(buff[0]);
	int j = 0;
	for (int i = 0; i < len; i++) {
		if (classify(buff[i]) == last) {
			out[j] = buff[i];
			j++;
		}
		else {
			out[j] = '\n';
			j++;
			out[j] = buff[i];
			j++;
			last = -last;
		}
	}

	out[j] = '\0';
	return out;
}

int classify(char x)
{
	if ('a' <= x && x <= 'z' || 'A' <= x && x <= 'Z')
		return 1;
	else if ('0' <= x && x <= '9')
		return -1;
	else return 0;
}

int Send(SOCKET s, char *buf, int len, int flags)
{
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
		ret = send(s, &buf[index], len, flags);
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