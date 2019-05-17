// Task2_Server.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>

#pragma comment (lib, "Ws2_32.lib")
// lỗi inet_addr và inet_ntoa đã lỗi thời nên chương trình yêu cầu dùng hàm inet_pton và inet_ntop
// vì dùng hàm gethostbyaddr cho nên phải khai báo cái dưới. nếu không chương trình bắt dùng hàm getnameinfo
#pragma warning(disable: 4996 )

#define SIZE 10240

char *DomainToIP(char *buff);
char *IPToHostname(char *ip);
int CheckIP(char *ip);
int CheckBuff(char *buff);
int Send(SOCKET s, char *buf, int len, int flags);
int Recv(SOCKET s, char *buf, int len, int flags);

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
	printf("Task 2: Server started!\n");

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
				char *rep = (char *)malloc(SIZE);
				if (CheckBuff(buff) == 0) {
					// không phải là địa chỉ nào cả, chứa ký tự đặc biệt != '.'
					strcpy(rep, "String not correct!\n");
				}
				else if (CheckBuff(buff) == 1) {
					// là tên miền
					strcpy(rep, DomainToIP(buff));
				}
				else {
					// là ip
					strcpy(rep, IPToHostname(buff));
				}
				//Echo to client
				ret = Send(connSock, rep, strlen(rep), 0);
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

// chuyển từ domain sang ip
char *DomainToIP(char *buff)
{
	char *out = (char *)malloc(SIZE);
	out[0] = '\0';
	strcat(out, "Official IP: ");

	addrinfo *result = NULL, *temp = NULL;
	sockaddr_in address;
	int rc = getaddrinfo(buff, "http", NULL, &result);
	if (rc != 0) {
		if (rc == WSAHOST_NOT_FOUND || rc == WSANO_DATA) {
			return "Not found information\n";
		}
		else {
			char str[12];
			sprintf(str, "Function getaddrinfo() failed with error: %ld\n", rc);
			return str;
		}
	}
	if (result != NULL) {
		// thêm Official IP
		// một ngày đẹp trời, nếu phân giải google.com thì bị lỗi chỗ memcpy và không biết vì sao
		// sau khi kiểm tra thì thấy tự nhiên ai_addr trả về giá trị không đúng kiểu và nó lỗi
		// thử các trang khác vẫn ok? rất khó hiểu
		memcpy(&address, result->ai_addr, result->ai_addrlen);
		char straddr[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &address.sin_addr, straddr, INET_ADDRSTRLEN);
		strcat(out, straddr);
		strcat(out, "\nAlias IP: \n");
		temp = result->ai_next;
		while (temp != NULL) {
			memcpy(&address, temp->ai_addr, temp->ai_addrlen);
			inet_ntop(AF_INET, &address.sin_addr, straddr, INET_ADDRSTRLEN);
			strcat(out, straddr);
			strcat(out, "\n");
			temp = temp->ai_next;
		}
	}

	freeaddrinfo(result);
	return out;
}

// chuyển từ ip sang hostname
char *IPToHostname(char *ip)
{
	if (CheckIP(ip) == -1)
		return "IP Address is invalid";
	char *out = (char *)malloc(SIZE);
	out[0] = '\0';
	strcat(out, "Official name: ");

	// dùng gethostbyaddr
	hostent *he;
	in_addr ipaddr;
	inet_pton(AF_INET, ip, &ipaddr);
	he = gethostbyaddr((char *)&ipaddr, sizeof(ipaddr), AF_INET);
	if (he == NULL) {
		int dwError = WSAGetLastError();
		if (dwError != 0) {
			if (dwError == WSAHOST_NOT_FOUND || dwError == WSANO_DATA) {
				return "Not found information\n";
			}
			else {
				char str[12];
				sprintf(str, "Function gethostbyaddr() failed with error: %ld\n", dwError);
				return str;
			}
		}
	}
	else {
		strcat(out, he->h_name);
		strcat(out, "\nAlias name: \n");
		for (char **pAlias = he->h_aliases; *pAlias != 0; pAlias++) {
			strcat(out, *pAlias);
			strcat(out, "\n");
		}
	}
	return out;
}

// check xem chuỗi client gửi sang có chứa kí tự đặc biệt hay không ngoài dấu chấm
int CheckBuff(char *buff)
{
	// có chứa kí tự đặc biệt
	for (int i = 0; i < strlen(buff); i++) {
		if ((buff[i] > '9' || buff[i] < '0') && buff[i] != '.' && buff[i] != '/' && (buff[i] > 'z' || buff[i] < 'a'))
			return 0;
	}
	// nếu ko chứa kí tự đặc biệt thì chứa 1 chữ cái sẽ được coi là domain
	for (int i = 0; i < strlen(buff); i++) {
		if (buff[i] <= 'z' && buff[i] >= 'a') {
			return 1;
		}
	}
	// nếu không phải thì là ip
	return -1;
}

// check định dạng ip
int CheckIP(char *ip)
{
	int len = strlen(ip), j = 0;
	char a[4] = "";
	int dot = 0;
	if (len > 15) return -1;
	for (int i = 0; i < len; i++) {
		if (ip[i] != '.') {
			a[j] = ip[i];
			j++;
		}
		else {
			dot++;
			a[j] = '\0';
			j = 0;
			if (atoi(a) < 0 || atoi(a) > 255) return -1;
		}
	}
	if (dot != 3) return -1;
	return 1;
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