#include "utils.h"

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