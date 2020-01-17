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


SOCKET sd;
struct Parameters {
	char command[DMAX_MSG];
	struct sockaddr_in* client;
};

void initWSA();
SOCKET createSocket();
void listenToClients(const SOCKET&);
int readMessage(const SOCKET&, const struct sockaddr_in*, const char*);
bool sendMessage(const SOCKET&, const struct sockaddr_in*, const char*);
void createCommandThread(const char*, const struct sockaddr_in*);
DWORD WINAPI executeCommand(LPVOID);
bool createFile(string);
bool appendToFile(string);


int main() {
	cout << "Initializing WSA\n";
	try {
		initWSA();
	}
	catch (exception e) {
		cout << "Something failed " << WSAGetLastError();
	}
	cout << "Creating socket\n";
	sd = createSocket();
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
	struct sockaddr_in* client = new struct sockaddr_in;
	int len, slen = sizeof(client);
	while (1) {
		cout << "Waiting for another client...\n";
		memset(buffer, 0, sizeof(buffer));

		len = readMessage(sd, client, buffer);
		if (len == SOCKET_ERROR)
		{
			cout << "read function failed: " << WSAGetLastError() << '\n';
			continue;
		}
		createCommandThread(buffer, client);
	}
}

void createCommandThread(const char* buffer, const struct sockaddr_in* client) {
	DWORD id;
	Parameters* threadParameters = new Parameters;
	threadParameters->client = new struct sockaddr_in;
	// buffer se schimba asa ca fac un nou array
	strcpy_s(threadParameters->command, buffer);
	memcpy(threadParameters->client, client, sizeof(*client));
	CreateThread(NULL, 0, executeCommand, (void*)threadParameters, 0, &id);
}

DWORD WINAPI executeCommand(LPVOID pPointer) {
	struct Parameters* params = (struct Parameters*) pPointer;
	string cmd = params->command;
	bool success = true;

	cout << "Executing command " << cmd << '\n';

	if (cmd.find("createfile") == 0) {
		if (createFile(cmd) != 0)
			success = false;
	}
	else if (cmd.find("append") == 0) {
		if (appendToFile(cmd) != 0)
			success = false;
	}

	if (success) {
		sendMessage(sd, params->client, "Command executed successfully");
		return 0;
	}
	else
	{
		sendMessage(sd, params->client, "Command failed");
		return -1;
	}
}

bool createFile(string cmd) {
	string fileName = cmd.substr(strlen("createfile "));
	cout << "Creating file " << fileName << '\n';
	HANDLE handle = CreateFile(
		LPCSTR(fileName.c_str()),
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		INVALID_HANDLE_VALUE
	);
	if (handle == INVALID_HANDLE_VALUE) {
		cout << "Failed to create file " << fileName << ": " << GetLastError() << "\n";
		return false;
	}
	CloseHandle(handle);
	cout << "File " << fileName << " created successfully\n";
	return true;
}

bool appendToFile(string cmd) {
	string aux = cmd.substr(strlen("append "));
	string continut = aux.substr(aux.find(" ") + 1);
	string fileName = aux.substr(0, aux.length() - continut.length());
	DWORD bytesWritten;

	cout << "Writing " << continut << " to " << fileName << '\n';
	HANDLE hFile = CreateFile(LPCSTR(fileName.c_str()),
		FILE_APPEND_DATA,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (hFile == INVALID_HANDLE_VALUE)
		return false;

	bool res = WriteFile(
		hFile,
		continut.c_str(),
		continut.length(),
		&bytesWritten,
		NULL
	);
	CloseHandle(hFile);
	return res;
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