// Task1_Client.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include "loginout.h"

#pragma comment (lib, "Ws2_32.lib")
#pragma warning(disable : 4996)


#define SIZE 2048

unsigned _stdcall sendThread(void *param);

int main(int argc, char *argv[])
{
	//Step 1: Inittiate WinSock
	WSADATA wsaData;
	WORD wVersion = MAKEWORD(2, 2);
	if (WSAStartup(wVersion, &wsaData))
		printf("Version is not supported.\n");
	printf("Task 1: Client started!\n");
	printf("Dang nhap theo cu phap: !LOGIN username password\n");
	printf("Dang xuat theo cu phap: !LOGOUT\n");
	printf("Ngat ket noi nhap vao: BYE\n");

	//Step 2: Construct socket
	SOCKET client;
	client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	////(optional) Set time-out for receiving
	//int tv = 10000; //Time-out interval: 10000ms
	//setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, (const char*)(&tv), sizeof(int));


	// Port and address
	int port = 5500;
	char addr[] = "127.0.0.1";
	if (argc == 3) { // 2 parameter
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
	// send thread
	_beginthreadex(0, 0, sendThread, (void *)client, 0, 0);
	// recv
	char buff[SIZE];
	int ret;
	while (1) {
		//Receive echo message
		ret = Recv(client, buff, SIZE, 0);
		if (ret == SOCKET_ERROR) {
			if (WSAGetLastError() == WSAECONNABORTED)
				break;
			printf("Error : %d", WSAGetLastError());
			system("pause");
		}
		else {
			buff[ret] = '\0';
			inet_ntop(AF_INET, &serverAddr.sin_addr, straddr, INET_ADDRSTRLEN);
			printf("Receive from server[%s:%d] % s\n", straddr, ntohs(serverAddr.sin_port), buff + 1);
		}
	}

	//Step 6: Close socket
	closesocket(client);

	//Step 7: Terminate Winsock
	WSACleanup();
	system("pause");
	return 0;
}

unsigned _stdcall sendThread(void *param)
{
	char buff[SIZE];
	int bytesent = 0, ret;
	SOCKET client = (SOCKET)param;
	while (1) {
		//Send message
		gets_s(buff, SIZE);
		rewind(stdin);

		ret = Send(client, buff, strlen(buff), 0);

		if (ret == SOCKET_ERROR)
			printf("Error! Cannot send mesage.");
		bytesent += strlen(buff) + 4;

		// end
		if (strcmp(buff, "BYE") == 0)
			break;
	}
	printf("Close Socket!\nBytes sent = %d\n", bytesent);
	closesocket(client);
	return 0;
}


