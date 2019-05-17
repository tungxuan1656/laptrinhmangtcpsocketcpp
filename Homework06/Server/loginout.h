#pragma once

#ifndef  DEF_LOGINOUT

#include <WinSock2.h>


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

int Send(SOCKET s, char *buf, int len, int flags);
int Recv(SOCKET s, char *buf, int len, int flags);
char *Process(char *buff, int *stt, account *acc);
char *Login(char *uspw, account *acc);
void UpdateAccount(account *acc);
account *LoadAccount();


#endif // ! DEF_LOGINOUT



