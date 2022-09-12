#pragma warning(suppress : 4996)
#pragma comment(lib, "Ws2_32.lib")
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <map>
using namespace std;

#define PORT    5555
#define BUFLEN  512
int   readFromClient(int fd, char* buf);
void  writeToClient(int fd, char* buf);

int  random(int imin, int imax) {
	double r = rand() / double(RAND_MAX);     // rand()%(imax - imin)
	int rr = r * (imax - imin) + imin;
	return rr;
}
bool find_vector(vector <int> mas, int a) {
	for (int i = 0; i < mas.size(); i++) {
		if (mas[i] == a) return true;
	}
	return false;
}
int find_el(vector<int> a, int b) {
	for (int i = 0; i < a.size(); i++) {
		if (a[i] == b) return i;
	}
	return -1;
}
struct field
{
	int** mas;
	int ships;
	vector <int> pleers;
	vector <int> pleers_fd;
	field(int n) {
		mas = new int* [10];
		for (int i = 0; i < 10; i++) {
			mas[i] = new int[10];
			for (int j = 0; j < 10; j++)
				mas[i][j] = -1;
		}
		int i = 0;
		while (i < n) {
			int a = random(0, 10);
			int b = random(0, 10);
			if (mas[a][b] == -1) {
				mas[a][b] = -3;
				i++;
			}
		}
		ships = n;
	}
	void fill(int s) {
		int i = 0;
		while (i < s) {
			int a = random(0, 10);
			int b = random(0, 10);
			if (mas[a][b] == -1) {
				mas[a][b] = -3;
				i++;
				ships++;
			}
		}

	}
	void clean() {
		for (int i = 0; i < 10; i++)
			for (int j = 0; j < 10; j++)
				mas[i][j] = -1;
		ships = 0;
	}
	bool shot(int a, int b, int fd) {
		if (mas[a][b] == -3) {
			mas[a][b] = fd;
			ships--;
			return true;
		}
		else if (mas[a][b] >= 0) {
			return false;
		}
		else if (mas[a][b] == -2) {
			return false;
		}
		else if (mas[a][b] == -1) {
			mas[a][b] = -1;
			return false;
		}
	}
	int skore(int fd) {
		int s = 0;
		for (int i = 0; i < 10; i++) {
			for (int j = 0; j < 10; j++) {
				if (mas[i][j] == fd) s++;
			}
		}
		return s;
	}
	void print() {
		cout << " ";
		for (int i = 0; i < 10; i++) {
			cout << i;
		}
		cout << endl;
		for (int i = 0; i < 10; i++) {
			cout << i;
			for (int j = 0; j < 10; j++) {
				if (mas[i][j] == -1) cout << " ";  // free cell
				if (mas[i][j] == -2) cout << "*"; // miss
				if (mas[i][j] >= 0) cout << "x";  // destroyed ship
				if (mas[i][j] == -3) cout << "#";  // ship
			}
			cout << endl;
		}
		cout << endl << "ships alife:" << ships << endl;

	}
};

field f(1);
int ans;
char answ[256];
int a;
int b;

int  main(void)
{
	f.print();
	int     i, err, opt = 1;
	int     sock, new_sock;
	fd_set  active_set, read_set;
	struct  sockaddr_in  addr;
	struct  sockaddr_in  client;
	char    buf[BUFLEN];
	socklen_t  size;

	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		printf("WSAStartup failed\n");
		return 1;
	}

	sock = socket(PF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		perror("Server: cannot create socket");
		exit(EXIT_FAILURE);
	}
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

	addr.sin_family = AF_INET;
	addr.sin_port = htons(PORT);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	err = bind(sock, (struct sockaddr*)&addr, sizeof(addr));
	if (err < 0) {
		perror("Server: cannot bind socket");
		WSACleanup();
		exit(EXIT_FAILURE);
	}

	err = listen(sock, 3);
	if (err < 0) {
		perror("Server: listen queue failure");
		WSACleanup();
		exit(EXIT_FAILURE);
	}

	FD_ZERO(&active_set);
	FD_SET(sock, &active_set);
	while (1) {
		read_set = active_set;
		if (select(FD_SETSIZE, &read_set, NULL, NULL, NULL) < 0) {
			perror("Server: select  failure");
			WSACleanup();
			exit(EXIT_FAILURE);
		}
		for (int j = 0; j < read_set.fd_count; j++) {
			i = read_set.fd_array[j];
			if (FD_ISSET(i, &read_set)) {
				if (i == sock) {
					size = sizeof(client);
					new_sock = accept(sock, (struct sockaddr*)&client, &size);
					if (new_sock < 0) {
						perror("accept");
						WSACleanup();
						exit(EXIT_FAILURE);
					}
					fprintf(stdout, "Server: connect from host %s, port %hu.\n",
						inet_ntoa(client.sin_addr),
						ntohs(client.sin_port));
					FD_SET(new_sock, &active_set);
				}
				else {
					
						err = readFromClient(i, buf);
						if (err < 0) {
							closesocket(i);
							FD_CLR(i, &active_set);
						}
						else {
							if (strstr(buf, "stop")) {
								closesocket(i);
								FD_CLR(i, &active_set);
							}
							else {
								writeToClient(i, buf);
							}
						}
				}
			}
		}
	}
	WSACleanup();
}

int  readFromClient(int fd, char* buf)
{
	int  nbytes;
	nbytes = recv(fd, buf, BUFLEN, 0);
	if (nbytes < 0) {
		perror("Server: read failure");
		return -1;
	}
	else if (nbytes == 0) {
		return -1;
	}
	else {
		return 0;
	}
}



void  writeToClient(int fd, char* buf)
{
		int nb;
		if (strstr(buf, "id")) {
			cout << "in id\n";
			stringstream ss;
			string s;
			int id;
			ss << buf;
			ss >> s >> id;
			f.pleers_fd.push_back(fd);
			f.pleers.push_back(id);
			send(fd, "1", BUFLEN, 0);
			send(fd, "id was readed", BUFLEN, 0);

			stringstream as;
			as << f.ships;
			char ans[BUFLEN];
			as >> ans;
			nb = send(fd, ans, BUFLEN, 0);
			cout<<"nb = "<<nb<<endl;
			cout << "send: " << ans << endl;
		}
		else if (strstr(buf, "score")) {
			cout << "in score\n";
			char* ms;
			ms = new char[BUFLEN];
			stringstream mess;
			mess << f.pleers.size();
			mess >> ms;
			send(fd, ms, sizeof(ms), 0);
			for (int l = 0; l < f.pleers.size(); l++) {
				stringstream ss;
				ss << "score of pleer " << f.pleers[l] << " is " << f.skore(f.pleers[l]);
				cout << ss.str() << endl;
				char* ch;
				ch = new char[BUFLEN];
				ss.getline(ch, BUFLEN);
				send(fd, ch, BUFLEN, 0);
			}
			stringstream as;
			as << f.ships;
			char ans[256];
			as >> ans;
			send(fd, ans, sizeof(ans), 0);
		}
		else {
			cout << "in else\n";
			stringstream ss;
			ss << buf;
			ss >> a >> b;
			if (f.shot(a, b, f.pleers[find_el(f.pleers_fd, fd)])) {
				ans = 1;
			}
			else {
				ans = 0;
			}
			f.print();
			int  nbytes;
			if (ans == 1) {
				send(fd, "1", sizeof("1"), 0);
				nbytes = send(fd, "ship was destroyed", BUFLEN, 0);
			}
			else {
				send(fd, "1", sizeof("1"), 0);
				nbytes = send(fd, "it was miss", BUFLEN, 0);
			}
			if (nbytes < 0) {
				perror("Server: write failure");
			}
			stringstream as;
			as << f.ships;
			char ans[BUFLEN];
			as >> ans;
			send(fd, ans, BUFLEN, 0);
		}
}