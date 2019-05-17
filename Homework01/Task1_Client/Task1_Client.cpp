// Task1_Client.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>

#pragma comment (lib, "Ws2_32.lib")
// lỗi inet_addr và inet_ntoa: chương trình yêu cầu dùng hàm inet_pton và inet_ntop

#define BUFF_SIZE 1000


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
	client = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
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

	//Step 4: Communicate with server
	char buff[BUFF_SIZE];
	int ret, serverAddrLen = sizeof(serverAddr);
	int bytesent = 0;
	while (1) {
		//Send message
		printf("Send to server: ");
		gets_s(buff, BUFF_SIZE);
		rewind(stdin);
		if (strlen(buff) == 0)
			break;
		ret = sendto(client, buff, strlen(buff), 0, (sockaddr *)&serverAddr, serverAddrLen);
		if (ret == SOCKET_ERROR)
			printf("Error! Cannot send mesage.");
		bytesent += strlen(buff);

		//Receive echo message
		ret = recvfrom(client, buff, BUFF_SIZE, 0, (sockaddr*)&serverAddr, &serverAddrLen);
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
		_strupr_s(buff, BUFF_SIZE);
	}

	printf("Bytes sent = %d\n", bytesent);

	//Step 5: Close socket
	closesocket(client);

	//Step 6: Terminate Winsock
	WSACleanup();
	system("pause");
	return 0;
}

