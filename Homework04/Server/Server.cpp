// Server.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <process.h>

#pragma comment (lib, "Ws2_32.lib")
#pragma warning(disable : 4996)
// lỗi inet_addr và inet_ntoa: chương trình yêu cầu dùng hàm inet_pton và inet_ntop

#define SIZE 1024

typedef struct request
{
	int key;
	int opcode;
} request;

typedef struct namefile
{
	char *filename;
	char *filenameout;
} namefile;

unsigned _stdcall echoThread(void *param);
int SendDataFile(char *fname, SOCKET s);
int Recv(SOCKET s, request *p_request, char *name);
void EncodeFile(request *p_request, namefile *p_name);

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
	printf("Server started!\n");

	//Step 5: Communicate with client
	sockaddr_in clientAddr;
	char buff[SIZE];
	int clientAddrLen = sizeof(clientAddr);

	while (1) {
		SOCKET connSock;
		// accept request
		connSock = accept(listenSock, (sockaddr *)&clientAddr, &clientAddrLen);
		printf("Accept: %d!\n", connSock);
		_beginthreadex(0, 0, echoThread, (void *)connSock, 0, 0);
	}

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

	namefile *p_name = (namefile *)malloc(sizeof(namefile));
	request *p_request = (request *)malloc(sizeof(request));
	srand(time(NULL));
	p_name->filename = (char *)malloc(100);
	int randomnumber = rand()*rand();
	sprintf(p_name->filename, "%d", randomnumber);

	// Receive message
	ret = Recv(connSock, p_request, p_name->filename);

	// process file
	EncodeFile(p_request, p_name);

	// send file encode
	SendDataFile(p_name->filenameout, connSock);

	remove(p_name->filename);
	remove(p_name->filenameout);

	shutdown(connSock, SD_SEND);
	closesocket(connSock);
	return 0;
}

// gửi đi dữ liệu của file fname
int SendDataFile(char *fname, SOCKET s)
{
	FILE *f = fopen(fname, "r");
	char *buff = (char *)malloc(SIZE + 1);
	buff[0] = 2;
	int temp, i, ret, len;
	while (1) {
		for (i = 3; i < 1024; i++) {
			temp = fgetc(f);
			if (temp == EOF) break;
			buff[i] = temp;
		}
		buff[i] = '\0';
		len = strlen(buff + 3);
		buff[1] = len / 256;
		buff[2] = len - buff[0] * 256;
		ret = send(s, buff, strlen(buff + 3) + 3, 0);
		if (ret == SOCKET_ERROR) return SOCKET_ERROR;
		if (temp == EOF) break;
	}
	if (len != 0) {
		buff[1] = 0;
		buff[2] = 0;
		buff[3] = '\0';
		ret = send(s, buff, strlen(buff + 3) + 3, 0);
	}
	fclose(f);
	return ret;
}

// nhận vào dữ liệu và lưu lại thành file tạm
int Recv(SOCKET s, request *p_request, char *name)
{
	char buff[SIZE + 1];
	int ret, lenght;
	while (1) {
		ret = recv(s, buff, SIZE, 0);
		if (ret == SOCKET_ERROR) return SOCKET_ERROR;
		buff[ret] = '\0';
		if (buff[0] == 0) {
			p_request->opcode = 0;
			p_request->key = (buff[3] > 0) ? buff[3] : buff[3] + 256;
		}
		else if (buff[0] == 1) {
			p_request->opcode = 1;
			p_request->key = (buff[3] > 0) ? buff[3] : buff[3] + 256;
		}
		else if (buff[0] == 2) {
			lenght = (((buff[1]>0) ? buff[1] : buff[1] + 256) * 256 + (buff[2]>0) ? buff[2] : buff[2] + 256);
			if (lenght == 0) {
				// bên client không làm gì cả.
				printf("Da nhan xong file!\n");
				break;
			}
			else {
				FILE *f = fopen(name, "a");
				if (f!= NULL) fputs(buff + 3, f);
				fclose(f);
			}
		}
		else {
			printf("Bao loi!\n");
		}
	}
	return ret;
}

// mã hóa hoặc giải mã theo request một file và lưu vào file tạm .enc
void EncodeFile(request *p_request, namefile *p_name)
{
	p_name->filenameout = (char *)malloc(100);
	sprintf(p_name->filenameout, "%s%s", p_name->filename, ".enc");
	FILE *fin = fopen(p_name->filename, "r");
	FILE *fout = fopen(p_name->filenameout, "w+");
	int temp = fgetc(fin), key = p_request->key;
	if (p_request->opcode == 0) {
		while (temp != EOF) {
			temp = (temp>0) ? temp : temp + 256;
			temp = (temp + key) % 256;
			fputc(temp, fout);
			temp = fgetc(fin);
		}
	}
	else {
		while (temp != EOF) {
			temp = (temp>0) ? temp : temp + 256;
			temp = (temp - key) % 256;
			fputc(temp, fout);
			temp = fgetc(fin);
		}
	}
	fclose(fin);
	fclose(fout);
	printf("Da encode xong!");
}