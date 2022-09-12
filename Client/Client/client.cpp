#pragma warning(suppress : 4996)
#pragma comment(lib, "Ws2_32.lib")

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <sstream>
#include <iostream>
#include <windows.h>
using namespace std;

// ќпределимс€ с портом, адресом сервера и другими константами.
// ¬ данном случае берем произвольный порт и адрес обратной св€зи
// (тестируем на одной машине).
#define  SERVER_PORT     5555
#define  SERVER_NAME    "127.0.0.1"
#define  BUFLEN          512

// ƒве вспомогательные функции дл€ чтени€/записи (см. ниже)
int  writeToServer(int fd, int ships);
int  readFromServer(int fd, int ships);
int  random(int imin, int imax) {
	double r = rand() / double(RAND_MAX);     // rand()%(imax - imin)
	int rr = r * (imax - imin) + imin;
	return rr;
}


int  main(int argc, char** argv)
{
	const char* serverName;
	serverName = (argc < 2) ? "127.0.0.1" : argv[1];

	int err;
	int sock;
	struct sockaddr_in server_addr;
	//struct hostent    *hostinfo;

	// инициализаци€ windows sockets
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		printf("WSAStartup failed\n");
		return -1;
	}

	// ѕолучаем информацию о сервере по его DNS имени
	// или точечной нотации IP адреса.
	//hostinfo = gethostbyname(SERVER_NAME);
	//if (hostinfo == NULL) {
	//	fprintf(stderr, "Unknown host %s.\n", SERVER_NAME);
	//	exit(EXIT_FAILURE);
	//}
	// можно было бы использовать GetAddrInfo()

	// «аполн€ем адресную структуру дл€ последующего
	// использовани€ при установлении соединени€
	server_addr.sin_family = PF_INET;
	server_addr.sin_port = htons(SERVER_PORT);
	//server_addr.sin_addr = *(struct in_addr*) hostinfo->h_addr;
	unsigned int iaddr;
	inet_pton(AF_INET, serverName, &iaddr);
	server_addr.sin_addr.s_addr = iaddr;

	// —оздаем TCP сокет.
	sock = socket(PF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		perror("Client: socket was not created");
		exit(EXIT_FAILURE);
	}

	// ”станавливаем соединение с сервером
	err = connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
	if (err < 0) {
		perror("Client:  connect failure");
		exit(EXIT_FAILURE);
	}
	fprintf(stdout, "Connection is ready\n");
	int i = 0;
	// ќбмениваемс€ данными
	while (1) {
		if (writeToServer(sock,i) < 0) break;
		if (readFromServer(sock,i) < 0) break;
		i++;
	}
	fprintf(stdout, "The end\n");

	// «акрываем socket
	closesocket(sock);
	WSACleanup();
	exit(EXIT_SUCCESS);
}



int  writeToServer(int fd, int i)
{
	Sleep(250);
	int   nbytes;
	char  buf[BUFLEN];
	if (i == 0) {
		fprintf(stdout, "Send to server > ");
		if (fgets(buf, BUFLEN, stdin) == nullptr) {
			printf("error\n");
		}
		buf[strlen(buf) - 1] = 0;
	}
	else {
		stringstream ss;

		ss << random(0, 10) << " " << random(0, 10);
		ss.getline(buf,BUFLEN);
		cout << "Send to server > " << buf << endl;
	}
	nbytes = send(fd, buf, strlen(buf) + 1, 0);
	if (nbytes < 0) { perror("write"); return -1; }
	if (strstr(buf, "stop")) return -1;
	return 0;
}


int  readFromServer(int fd,int ships)
{
	stringstream ss;
	int   nbytes;
	char  buf[BUFLEN];

	nbytes = recv(fd, buf, BUFLEN, 0);
	int n;
	ss << buf;
	ss >> n;

	for (int i = 0; i < n; i++) {
		nbytes = recv(fd, buf, BUFLEN, 0);
		cout <<buf << endl;
	}

	nbytes = recv(fd, buf, BUFLEN, 0);
	stringstream as;
	as << buf;
	int l;
	as >> l;
	if (l == 0) {
		stringstream as;
		send(fd, "score", sizeof("score"), 0);
		nbytes = recv(fd, buf, BUFLEN, 0);
		int l;
		as << buf;
		as >> l;
		
		for (int i = 0; i < l; i++) {
			nbytes = recv(fd, buf, BUFLEN, 0);
			cout << buf << endl;
		}
		return -1;
	}

	return 0;
}

