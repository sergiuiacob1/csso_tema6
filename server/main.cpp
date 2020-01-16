#include <iostream>
#include <string>
#include <Winsock2.h>
#include <windows.h>
#include <errno.h>
#include <Ws2tcpip.h>
#pragma comment(lib,"ws2_32.lib") //Winsock Library
#define PORT 3001
#define DMAX_MSG 2048
#define CHUNK_SIZE 1024

using namespace std;

struct Parameters {
	char command[DMAX_MSG];
	struct sockaddr_in* client;
};

void initWSA();
SOCKET createSocket();
void listenToClients(const SOCKET&);
int readMessage(const SOCKET&, const struct sockaddr_in*, const char*);
bool sendMessage(const SOCKET&, const struct sockaddr_in*, const char*);
void createCommandThread(const Parameters*);
DWORD WINAPI executeCommand(LPVOID);


int main() {
	cout << "Initializing WSA\n";
	try {
		initWSA();
	}
	catch (exception e) {
		cout << "Something failed " << WSAGetLastError();
	}
	cout << "Creating socket\n";
	SOCKET sd = createSocket();
	if (sd == NULL) {
		cout << "Could not create socked: " << WSAGetLastError() << '\n';
		return -1;
	}
	listenToClients(sd);

	// cleanup
	closesocket(sd);
	WSACleanup();
	return 0;
}

int readMessage(const SOCKET& sd, const struct sockaddr_in* conn, char* buffer) {
	int slen = sizeof(*conn);
	int len = recvfrom(sd, buffer, DMAX_MSG, 0, (struct sockaddr*)  conn, &slen);
	if (len != SOCKET_ERROR)
		buffer[len] = 0;
	return len;
}

bool sendMessage(const SOCKET& sd, const struct sockaddr_in* conn, const char* messageToSend) {
	int slen = sizeof(*conn);
	int recv_len = strlen(messageToSend);
	if (sendto(sd, messageToSend, recv_len, 0, (struct sockaddr*) conn, slen) == SOCKET_ERROR)
		return false;
	return true;
}

void listenToClients(const SOCKET& sd) {
	char buffer[DMAX_MSG];
	char ipbuf[INET_ADDRSTRLEN];
	struct sockaddr_in* client = new struct sockaddr_in;
	int len, slen = sizeof(client);
	struct Parameters* threadParameters;
	while (1) {
		cout << "Waiting for another client...\n";
		memset(buffer, 0, sizeof(buffer));

		len = readMessage(sd, client, buffer);
		if (len == SOCKET_ERROR)
		{
			cout << "read function failed: " << WSAGetLastError() << '\n';
			continue;
		}

		// buffer se poate schimba asa ca fac 
		threadParameters = new Parameters;
		threadParameters->client = new struct sockaddr_in;
		strcpy_s(threadParameters->command, buffer);
		memcpy(threadParameters->client, client, sizeof(*client));
		createCommandThread(threadParameters);

		if (sendMessage(sd, client, "Hello there") == false)
			cout << "Could not send message :(\n";
		else
			cout << "Message sent to client\n";
	}
}

void createCommandThread(const Parameters* threadParameters) {
	DWORD id;
	CreateThread(NULL, 0, executeCommand, (void*)threadParameters, 0, &id);
}

DWORD WINAPI executeCommand(LPVOID pPointer) {
	struct Parameters* params = (struct Parameters*) pPointer;

	cout << "Executing command " << params->command << '\n';

	return 0;
}

void initWSA() {
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
}

SOCKET createSocket() {
	SOCKET sd;
	struct sockaddr_in server;

	// folosesc UDP (SOCK_DGRAM)
	if ((sd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		cout << "Could not create UDP socket: " << WSAGetLastError() << '\n';
		return NULL;
	}

	memset(&server, 0, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	server.sin_port = htons(PORT);

	if (bind(sd, (struct sockaddr*) & server, sizeof(struct sockaddr)) == -1) {
		perror("[server]Eroare la bind().\n");
		return NULL;
	}

	return sd;
}