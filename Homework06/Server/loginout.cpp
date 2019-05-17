#include "stdafx.h"
#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>

#pragma comment (lib, "Ws2_32.lib")
#pragma warning(disable : 4996)

typedef struct account
{
	char user[15];
	char pw[15];
	int stt;
	int wrong;
	struct account *next = NULL;
} account;

typedef struct CODE
{
	char LOGIN = '1';
	char LOGOUT = '2';
	char MESSAGE = '3';
	char LOGGED = '1';
	char GUEST = '2';
} CODE;

#define BUFF_SIZE 2048

int Send(SOCKET s, char *buf, int len, int flags);
int Recv(SOCKET s, char *buf, int len, int flags);
char *Process(char *buff, int *stt, account *acc);
char *Login(char *uspw, account *acc);
void UpdateAccount(account *acc);
account *LoadAccount();

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
	char *out = (char *)malloc(BUFF_SIZE);
	CODE c;
	int code = buff[0];
	if (code == c.LOGIN) {
		if (*pstt == c.LOGGED) {
			char s[] = "You were logged!";
			strcpy(out, s);
		}
		else { // GUEST
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
		else { // LOGOUT
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
		strcpy(out, "ERROR! Syntactically not correct");
		return out;
	}
	memcpy(user, uspw, i);
	user[i] = '\0';
	memcpy(pw, uspw + i + 1, strlen(uspw) - i);
	pw[strlen(uspw) - i - 1] = '\0';

	strcpy(out, "Wrong account!");

	for (account *p = acc->next; p != NULL; p = p->next) {
		if (strcmp(user, p->user) == 0) {
			if (p->stt == 0) {
				strcpy(out, "Your account is Locked!");
			}
			else {
				if (strcmp(pw, p->pw) == 0) {
					p->wrong = 0;
					strcpy(out, "Login successful");
				}
				else {
					(p->wrong)++;
					strcpy(out, "Password is wrong!");
					if (p->wrong == 3) {
						p->stt = 0;
						UpdateAccount(acc);
						strcat(out, "Account has been locked ");
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
	char *sendbuf = (char *)malloc(BUFF_SIZE);
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

	len = strlen(sendbuf);

	char size[4];
	int index = 0, ret;

	size[0] = len / (128 * 128 * 128);
	size[1] = (len - size[0] * 128 * 128 * 128) / (128 * 128);
	size[2] = (len - size[0] * 128 * 128 * 128 - size[1] * 128 * 128) / 128;
	size[3] = len - size[0] * 128 * 128 * 128 - size[1] * 128 * 128 - size[2] * 128;


	// send size
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