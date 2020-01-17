#pragma once
#include <iostream>
#include <string>
#include <Winsock2.h>
#include <windows.h>
#include <errno.h>
#include <Ws2tcpip.h>
#define DMAX_MSG 2048
#define CHUNK_SIZE 1024

int readMessage(const SOCKET&, const struct sockaddr_in*, char*);
bool sendMessage(const SOCKET&, const struct sockaddr_in*, char*);
