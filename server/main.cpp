#include <iostream>
#include <string>
#include <Winsock2.h>
#include <windows.h>
#include <errno.h>
#include <Ws2tcpip.h>
#include <stdio.h>
#include <Strsafe.h>
#include <Pathcch.h>
#include <Shlwapi.h>
#include <fileapi.h>
#pragma comment(lib,"ws2_32.lib") //Winsock Library
#define PORT 3001
#define DMAX_MSG 2048
#define CHUNK_SIZE 1024
#define MAX_PATH_LENGTH 1024

using namespace std;

string clientResponse;


SOCKET sd;
struct Parameters {
	char command[DMAX_MSG];
	struct sockaddr_in* client;
};

LPCSTR pathToDirectoryParent = "C:\\Users\\sergiu\\Desktop";

void initWSA();
SOCKET createSocket();
void listenToClients(const SOCKET&);
int readMessage(const SOCKET&, const struct sockaddr_in*, const char*);
bool sendMessage(const SOCKET&, const struct sockaddr_in*, const char*);
void createCommandThread(const char*, const struct sockaddr_in*);
DWORD WINAPI executeCommand(LPVOID);
bool createFile(string);
bool appendToFile(string);
bool deleteFile(string);
bool createRegistryKey(string);
bool deleteRegistryKey(string);
bool runProcess(string);
bool listDir(string);
void unifyPaths(LPSTR, LPCSTR, LPCSTR, bool starAtTheEnd = true);
void listFiles(LPCSTR, int);
bool isDirectory(WIN32_FIND_DATA ffd) {
	return (ffd.dwFileAttributes) & FILE_ATTRIBUTE_DIRECTORY;
}


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
	clientResponse.clear();

	cout << "Executing command " << cmd << '\n';

	if (cmd.find("createfile") == 0) {
		success = createFile(cmd);
	}
	else if (cmd.find("append") == 0) {
		success = appendToFile(cmd);
	}
	else if (cmd.find("deletefile") == 0) {
		success = deleteFile(cmd);
	}
	else if (cmd.find("createkey") == 0) {
		success = createRegistryKey(cmd);
	}
	else if (cmd.find("deletekey") == 0) {
		success = deleteRegistryKey(cmd);
	}
	else if (cmd.find("run") == 0) {
		success = runProcess(cmd);
	}
	else if (cmd.find("listdir") == 0) {
		success = listDir(cmd);
	}

	if (success) {
		if (cmd.find("listdir") == 0)
			sendMessage(sd, params->client, clientResponse.c_str());
		else
			sendMessage(sd, params->client, "Command executed successfully\n");
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

bool deleteFile(string cmd) {
	string fileName = cmd.substr(strlen("createfile "));
	cout << "Deleting file " << fileName << '\n';
	return DeleteFile(fileName.c_str());
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

bool createRegistryKey(string cmd) {
	DWORD disposition = 0;
	HKEY hKey;
	string reg = cmd.substr(strlen("createkey "));
	return RegCreateKeyEx(
		HKEY_CURRENT_USER,
		reg.c_str(),
		0,
		NULL,
		REG_OPTION_NON_VOLATILE,
		KEY_READ | KEY_WRITE,
		NULL,
		&hKey,
		&disposition
	);
}

bool deleteRegistryKey(string cmd) {
	HKEY hKey;
	string reg = cmd.substr(strlen("deletekey "));

	if (RegOpenKeyEx(HKEY_CURRENT_USER, 0, 0, KEY_READ, &hKey) != 0)
		return false;
	return RegDeleteKey(hKey, reg.c_str());
}

bool runProcess(string cmd) {
	PROCESS_INFORMATION pi;
	STARTUPINFO si;
	memset(&si, 0, sizeof(si));
	si.cb = sizeof(si);
	string processPath = cmd.substr(strlen("run "));

	return CreateProcess(processPath.c_str(), NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
}

bool listDir(string cmd) {
	string path = cmd.substr(strlen("listdir "));
	listFiles(path.c_str(), 0);
	return true;
}

void listFiles(LPCSTR pathSuffix, int tabs) {
	WIN32_FIND_DATA ffd;
	HANDLE hFile;
	LPSTR currentPath = new char[MAX_PATH_LENGTH];

	unifyPaths(currentPath, pathToDirectoryParent, pathSuffix);

	hFile = FindFirstFileA(currentPath, &ffd);
	if (hFile == INVALID_HANDLE_VALUE) {
		printf("Could not acquire handle for %s\n", currentPath);
		return;
	}
	do {
		if (strcmp(ffd.cFileName, ".") == 0 || strcmp(ffd.cFileName, "..") == 0)
			continue;
		if (isDirectory(ffd)) {
			LPSTR newPath = new char[MAX_PATH_LENGTH];
			unifyPaths(newPath, pathSuffix, ffd.cFileName, false);

			listFiles((LPCSTR)newPath, tabs + 1);
		}
		else
		{
			LPSTR newPath = new char[MAX_PATH_LENGTH];
			unifyPaths(newPath, pathSuffix, ffd.cFileName, false);

			for (int i = 0; i < tabs; ++i)
				clientResponse += "    ";
			clientResponse += newPath;
			clientResponse += "\n";
		}
	} while (FindNextFileA(hFile, &ffd));

	FindClose(hFile);
}

void unifyPaths(LPSTR newPath, LPCSTR path1, LPCSTR path2, bool starAtTheEnd) {
	// copy without the * at the end

	size_t len = strlen(path1);
	if (len > 0) {
		if (path1[len - 1] == '*')
			--len;

		StringCchCopyN(newPath, MAX_PATH_LENGTH, path1, len);
		if (newPath[strlen(newPath) - 1] != '\\')
			StringCchCat(newPath, MAX_PATH_LENGTH, "\\");
	}
	else {
		StringCchCopy(newPath, MAX_PATH_LENGTH, "");
	}

	len = strlen(path2);
	if (path2[len - 1] == '\\')
		--len;
	// append the second string without '\' at the end
	StringCchCatN(newPath, MAX_PATH_LENGTH, path2, len);
	if (starAtTheEnd == true) {
		if (newPath[strlen(newPath) - 1] == '\\')
			StringCchCat(newPath, MAX_PATH_LENGTH, "*");
		else
			StringCchCat(newPath, MAX_PATH_LENGTH, "\\*");
	}
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