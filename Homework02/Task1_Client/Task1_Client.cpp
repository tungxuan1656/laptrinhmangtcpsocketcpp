// Task1_Client.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>

#pragma comment (lib, "Ws2_32.lib")
// lỗi inet_addr và inet_ntoa: chương trình yêu cầu dùng hàm inet_pton và inet_ntop

#define SIZE 100000

int Send(SOCKET s, char *buf, int len, int flags);
int Recv(SOCKET s, char *buf, int len, int flags);


int main(int argc, char *argv[])
{
	//Step 1: Inittiate WinSock
	WSADATA wsaData;
	WORD wVersion = MAKEWORD(2, 2);
	if (WSAStartup(wVersion, &wsaData))
		printf("Version is not supported.\n");
	printf("Task 1: Client started!\n");

	//Step 2: Construct socket
	SOCKET client;
	client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	//(optional) Set time-out for receiving
	int tv = 10000; //Time-out interval: 10000ms
	setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, (const char*)(&tv), sizeof(int));


	// Port and address
	int port = 5500;
	char addr[] = "127.0.0.1";
	if (argc == 3) { // đủ 2 tham số dòng lệnh
		strcpy_s(addr, argv[1]);
		port = atoi(argv[2]);
	}

	//Step 3: Specify server address
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);
	inet_pton(AF_INET, addr, &serverAddr.sin_addr.s_addr);
	char straddr[INET_ADDRSTRLEN];

	// Step 4: Request to connect server
	if (connect(client, (sockaddr *)&serverAddr, sizeof(serverAddr))) {
		printf("Error! Cannot connect server.\n");
		system("pause");
		return 0;
	}

	//Step 5: Communicate with server
	char buff[SIZE];
	int ret;
	int bytesent = 0;
	while (1) {
		//Send message
		printf("Send to server: ");
		gets_s(buff, SIZE);
		rewind(stdin);

		ret = Send(client, buff, strlen(buff), 0);

		if (ret == SOCKET_ERROR)
			printf("Error! Cannot send mesage.");
		bytesent += strlen(buff) + 4;

		// kết thúc khi gửi xâu rỗng cho Server
		if (strlen(buff) == 0)
			break;

		//Receive echo message
		ret = Recv(client, buff, SIZE, 0);
		if (ret == SOCKET_ERROR) {
			if (WSAGetLastError() == WSAETIMEDOUT)
				printf("Time-out!\n");
			else
				printf("Error! Cannot receive message.\n");
		}
		else {
			buff[ret] = '\0';
			// thay cho inet_ntoa(serverAddr.sin_addr)
			inet_ntop(AF_INET, &serverAddr.sin_addr, straddr, INET_ADDRSTRLEN);
			printf("Receive from server[%s:%d] % s\n", straddr, ntohs(serverAddr.sin_port), buff);
		}
	}

	//Step 6: Close socket
	closesocket(client);
	printf("Close Socket!\nBytes sent = %d\n", bytesent);

	//Step 7: Terminate Winsock
	WSACleanup();
	system("pause");
	return 0;
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

