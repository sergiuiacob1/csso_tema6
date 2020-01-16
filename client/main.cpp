#include <iostream>
#include <string>
#include <Winsock2.h>
#include <windows.h>
#include <errno.h>
#include <Ws2tcpip.h>
#pragma comment(lib,"ws2_32.lib") //Winsock Library
#define SERVER_ADDRESS "127.0.0.1"
#define PORT 3001
#define DMAX_MSG 2048
#define CHUNK_SIZE 1024

using namespace std;

void initWSA();
void createConnectionToServer();
int readMessage(const SOCKET&, const struct sockaddr_in*, const char*);
bool sendMessage(const SOCKET&, const struct sockaddr_in*, const char*);
void sendCommands();

SOCKET sd;
struct sockaddr_in server;

int main() {
	cout << "Initializing WSA\n";
	try {
		initWSA();
	}
	catch (exception e) {
		cout << "Something failed " << GetLastError();
	}
	cout << "Connecting to server...\n";
	try {
		createConnectionToServer();
	}
	catch (exception e) {
		cout << "Error: " << e.what() << '\n';
		return -1;
	}

	// send commands
	sendCommands();

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

void sendCommands() {
	char buffer[DMAX_MSG];
	int len, slen = sizeof(server);
	while (1) {
		cout << "Enter command: ";
		cin >> buffer;

		// trimit catre server
		if (sendto(sd, buffer, strlen(buffer), 0, (struct sockaddr*) & server, slen) == SOCKET_ERROR)
		{
			cout << "Could not send message to server: " << GetLastError() << "\n";
			continue;
		}
		cout << "Command sent\n";

		// primesc un raspuns

		if ((len = recvfrom(sd, buffer, DMAX_MSG, 0, (struct sockaddr*) & server, &slen)) == SOCKET_ERROR)
		{
			cout << "recvfrom() function failed: " << GetLastError() << '\n';
			continue;
		}

		cout << "Response: " << buffer << '\n';
	}
}

void initWSA() {
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
}

void createConnectionToServer() {
	if ((sd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == SOCKET_ERROR)
	{
		cout << "Could not create UDP socket: " << GetLastError() << '\n';
		throw exception("Socket creation failed");
	}

	//setup address structure
	memset((char*)&server, 0, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_port = htons(PORT);
	// deprecated
	// server.sin_addr.s_addr = inet_addr(SERVER_ADDRESS);
	InetPton(AF_INET, TEXT(SERVER_ADDRESS), &server.sin_addr.s_addr);
}