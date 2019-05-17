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

#define BUFF_SIZE 1000

char *DomainToIP(char *buff);
char *IPToHostname(char *ip);
int CheckIP(char *ip);
int CheckBuff(char *buff);

int main(int argc, const char *argv[])
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
	//serverAddr.sin_addr.s_addr = inet_addr(add);
	inet_pton(AF_INET, addr, &serverAddr.sin_addr.s_addr);
	if (bind(server, (sockaddr *)&serverAddr, sizeof(serverAddr))) {
		printf("Error! Cannot bind this address.");
		system("pause");
		return 0;
	}
	printf("Task 2: Server started!\n");

	//printf("%s", DomainToIP("bing.com"));
	//printf("%s", IPToHostname("a.2.3.4"));

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
			// chuyển từ IP 32 bit sang chuỗi dùng inet_ntop (ko dùng inet_ntoa vì chạy lên nó bảo thế)
			inet_ntop(AF_INET, &clientAddr.sin_addr, straddr, INET_ADDRSTRLEN);
			printf("Receive from client[%s:%d] %s\n", straddr, ntohs(clientAddr.sin_port), buff);
			
			// Xử lý tin nhắn của client
			// Không dùng được regular -> xử lý chay
			char rep[BUFF_SIZE];
			if (CheckBuff(buff) == 0) {
				// không phải là địa chỉ nào cả, chứa ký tự đặc biệt != '.'
				strcpy_s(rep, "String not correct!\n");
			}
			else if (CheckBuff(buff) == 1) {
				// là tên miền
				strcpy_s(rep, DomainToIP(buff));
			}
			else {
				// là ip
				strcpy_s(rep, IPToHostname(buff));
			}
			
			//Echo to client
			ret = sendto(server, rep, strlen(rep), 0, (SOCKADDR *)&clientAddr, sizeof(clientAddr));
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

// chuyển từ domain sang ip
char *DomainToIP(char *buff)
{
	char out[BUFF_SIZE] = "";
	strcat_s(out, "Official IP: ");

	addrinfo *result = NULL, *temp = NULL;
	sockaddr_in address;
	int rc = getaddrinfo(buff, NULL, NULL, &result);
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
		memcpy(&address, (sockaddr_in *)result->ai_addr, result->ai_addrlen);
		char straddr[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &address.sin_addr, straddr, INET_ADDRSTRLEN);
		strcat_s(out, straddr);
		strcat_s(out, "\nAlias IP: \n");
		temp = result->ai_next;
		while (temp != NULL) {
			memcpy(&address, temp->ai_addr, temp->ai_addrlen);
			inet_ntop(AF_INET, &address.sin_addr, straddr, INET_ADDRSTRLEN);
			strcat_s(out, straddr);
			strcat_s(out, "\n");
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
	char out[BUFF_SIZE] = "";
	char re[BUFF_SIZE] = "";
	strcat_s(out, "Official name: ");
	
	/*
	sử dụng gethostbyaddr thì nó bắt dùng getnameinfo.
	nhưng dùng getnameinfo thì giá trị trả về chỉ có một host name mà ko có alias name.
	trên trang của microsoft thì có dòng: Note  The getnameinfo function cannot be used to resolve alias names.
	*/
	 
	/*sockaddr_in sa;
	sa.sin_family = AF_INET;
	inet_pton(AF_INET, ip, &sa.sin_addr.s_addr);
	int rc = getnameinfo((sockaddr *)&sa, sizeof(sa), re, sizeof(re), NULL, NULL, 0);
	if (rc != 0)
		return "IP Address is invalid";
	if (strcmp(re, ip) != 0) {
		strcat_s(out, re);
	}
	else {
		strcpy_s(out, "Not found information");
	}*/
	
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
		strcat_s(out, he->h_name);
		strcat_s(out, "\nAlias name: \n");
		for (char **pAlias = he->h_aliases; *pAlias != 0; pAlias++) {
			strcat_s(out, *pAlias);
			strcat_s(out, "\n");
		}
	}
	return out;
}

// check xem chuỗi client gửi sang có chứa kí tự đặc biệt hay không ngoài dấu chấm
int CheckBuff(char *buff)
{
	// có chứa kí tự đặc biệt
	for (int i = 0; i < strlen(buff); i++) {
		if ((buff[i] > '9' || buff[i] < '0') && buff[i] != '.' && (buff[i] > 'z' || buff[i] < 'a'))
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

