// Task1_Server.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>

#pragma comment (lib, "Ws2_32.lib")
// lỗi inet_addr và inet_ntoa: chương trình yêu cầu dùng hàm inet_pton và inet_ntop

#define BUFF_SIZE 1000

char *EditBuff(char *buff);
int classify(char x);

int main(int argc,const char *argv[])
{
	//Step 1: Inittiate WinSock
	WSADATA wsaData;
	WORD wVersion = MAKEWORD(2, 2);
	if (WSAStartup(wVersion, &wsaData))
		printf("Version is not supported\n");

	//Step 2: Construct socket
	SOCKET server;
	server = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	// Port and address
	int port = 5500;
	char addr[] = "127.0.0.1";
	if (argc == 2) { // đủ 1 tham số dòng lệnh
		port = atoi(argv[1]);
	}

	//Step 3: Bind address to socket
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);
	inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr.s_addr);
	if (bind(server, (sockaddr *)&serverAddr, sizeof(serverAddr))) {
		printf("Error! Cannot bind this address.");
		system("pause");
		return 0;
	}
	printf("Task 1: Server started!");

	//Step 4: Communicate with client
	sockaddr_in clientAddr;
	char straddr[INET_ADDRSTRLEN];
	char buff[BUFF_SIZE];
	int ret, clientAddrLen = sizeof(clientAddr);
	while (1) {
		//Receive message
		ret = recvfrom(server, buff, BUFF_SIZE, 0, (sockaddr *)&clientAddr, &clientAddrLen);
		if (ret == SOCKET_ERROR)
			printf("Error : %d", WSAGetLastError());
		else {
			buff[ret] = '\0';
			inet_ntop(AF_INET, &clientAddr.sin_addr, straddr, INET_ADDRSTRLEN);
			printf("Receive from client[%s:%d] %s\n", straddr, ntohs(clientAddr.sin_port), buff);
			// Xử lý tin nhắn của client
			// Không dùng được regular -> xử lý chay
			char *out = EditBuff(buff);

			//Echo to client
			ret = sendto(server, out, strlen(out), 0, (SOCKADDR *)&clientAddr, sizeof(clientAddr));
			if (ret == SOCKET_ERROR)
				printf("Error: %", WSAGetLastError());
		}
	}

	//Step 5: Close socket
	closesocket(server);

	//Step 6: Terminate Winsock
	WSACleanup();
	system("pause");
	return 0;
}

char *EditBuff(char *buff)
{
	char out[BUFF_SIZE];
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

