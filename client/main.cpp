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
void sendCommand(string);

SOCKET sd;
struct sockaddr_in server;

int main(int argc, char* argv[]) {
	string command;
	cout << "Initializing WSA\n";
	try {
		initWSA();
	}
	catch (exception e) {
		cout << "Something failed " << WSAGetLastError();
	}
	cout << "Connecting to server...\n";
	try {
		createConnectionToServer();
	}
	catch (exception e) {
		cout << "Error: " << e.what() << '\n';
		return -1;
	}

	for (int i = 1; i < argc; ++i) {
		command += argv[i];
		if (i < argc - 1)
			command += " ";
	}

	// send command
	sendCommand(command);

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

void sendCommand(string command) {
	char buffer[DMAX_MSG];
	int len, slen = sizeof(server);

	// trimit catre server
	if (sendMessage(sd, &server, command.c_str()) == false)
	{
		cout << "Could not send message to server: " << WSAGetLastError() << "\n";
		return;
	}
	cout << "Command sent\n";

	// primesc un raspuns
	len = readMessage(sd, &server, buffer);
	if (len == SOCKET_ERROR)
	{
		cout << "recvfrom() function failed: " << WSAGetLastError() << '\n';
		return;
	}

	cout << "Response:\n" << buffer << '\n';

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