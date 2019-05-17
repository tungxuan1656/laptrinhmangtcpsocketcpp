// Client.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>

#pragma comment (lib, "Ws2_32.lib")
#pragma warning(disable : 4996)
// lỗi inet_addr và inet_ntoa: chương trình yêu cầu dùng hàm inet_pton và inet_ntop

#define SIZE 1024

typedef struct namefile
{
	char *filename;
	char *filenameout;
} namefile;

typedef struct request
{
	int key;
	int opcode;
} request;

int SendRequest(SOCKET s);
void InputFile(namefile *p_name);
int SendDataFile(char *fname, SOCKET s);
int Recv(SOCKET s, request *p_request, char *name);

int main(int argc, char *argv[])
{
	//Step 1: Inittiate WinSock
	WSADATA wsaData;
	WORD wVersion = MAKEWORD(2, 2);
	if (WSAStartup(wVersion, &wsaData))
		printf("Version is not supported.\n");
	printf("Client started!\n");

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

	namefile *p_name = (namefile *)malloc(sizeof(namefile));
	request *p_request = (request *)malloc(sizeof(request));

	// request and key
	ret = SendRequest(client);
	if (ret == SOCKET_ERROR) {
		printf("Error: %d", WSAGetLastError);
		return 1;
	}
	//nhập vào file
	InputFile(p_name);

	ret = SendDataFile(p_name->filename, client);
	if (ret == SOCKET_ERROR) {
		printf("Error: %d", WSAGetLastError);
		return 1;
	}
	
	// chờ nhận kết quả
	ret = Recv(client, p_request, p_name->filenameout);

	//Step 6: Close socket
	closesocket(client);

	//Step 7: Terminate Winsock
	WSACleanup();
	system("pause");
	return 0;
}

// gửi đi yêu cầu mã hóa hoặc giải mã kèm key
int SendRequest(SOCKET s)
{
	int opcode = 0;
	printf("Menu:\n0: Ma hoa\n1: Giai ma\n");
	do {
		printf("Lua chon: ");
		scanf("%d", &opcode);
	} while (opcode != 0 && opcode != 1);
	int key = 0;
	do {
		printf("Nhap gia tri khoa: ");
		scanf("%d", &key);
	} while (key > 255 || key < 0);

	char *buff = (char *)malloc(SIZE);

	buff[0] = opcode;
	buff[1] = 0;
	buff[2] = 0;
	buff[3] = key;
	buff[4] = '\0';

	return send(s, buff, strlen(buff + 3) + 3, 0);
}

// nhập vào tên file và lưu vào namefile
void InputFile(namefile *p_name)
{
	char *filename = (char *)malloc(100);
	printf("Nhap file can gui: ");
	rewind(stdin);
	scanf("%s", filename);
	FILE *f = fopen(filename, "r");
	p_name->filename = filename;
	p_name->filenameout = (char *)malloc(100);
	sprintf(p_name->filenameout, "%s%s", p_name->filename, ".enc");
	FILE *f1 = fopen(p_name->filenameout, "w+");
	if (f == NULL || f1 == NULL) {
		printf("Khong the mo file!\n");
		system("pause");
		exit(1);
	}
	fclose(f);
	fclose(f1);
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
		ret = send(s, buff, strlen(buff +3) + 3, 0);
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
				fputs(buff + 3, f);
				fclose(f);
			}
		}
		else {
			printf("Bao loi!\n");
		}
	}
	return ret;
}
