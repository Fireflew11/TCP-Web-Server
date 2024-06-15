#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <iostream>
using namespace std;
#pragma comment(lib, "Ws2_32.lib")
#include "httpCommands.h"
#include <winsock2.h>
#include <string.h>
#include <time.h>
#include <ctime>
#include <unordered_map>
#include <sstream>
#include <algorithm>


bool addSocket(SOCKET id, int what);
void removeSocket(int index);
void acceptConnection(int index);
int readHeader(SocketState& state);
int extractContentLength(SocketState& state);
int readBody(SocketState& state);
int receiveOneMessage(SocketState& state);
void receiveMessage(int index);
void sendMessage(int index);


struct SocketState sockets[MAX_SOCKETS] = { 0 };
int socketsCount = 0;
const int TIME_FOR_TIMEOUT = 120;
const timeval MAX_TIME_FOR_SELECT = { TIME_FOR_TIMEOUT, 0 }; // 2 minutes timeout


void main()
{
	//initialize path
	initializeBaseDirectory();

	// Initialize Winsock (Windows Sockets).

	// Create a WSADATA object called wsaData.
	// The WSADATA structure contains information about the Windows 
	// Sockets implementation.
	WSAData wsaData;

	// Call WSAStartup and return its value as an integer and check for errors.
	// The WSAStartup function initiates the use of WS2_32.DLL by a process.
	// First parameter is the version number 2.2.
	// The WSACleanup function destructs the use of WS2_32.DLL by a process.
	if (NO_ERROR != WSAStartup(MAKEWORD(2, 2), &wsaData))
	{
		cout << "Web Server: Error at WSAStartup()\n";
		return;
	}

	// Server side:
	// Create and bind a socket to an internet address.
	// Listen through the socket for incoming connections.

	// After initialization, a SOCKET object is ready to be instantiated.

	// Create a SOCKET object called listenSocket. 
	// For this application:	use the Internet address family (AF_INET), 
	//							streaming sockets (SOCK_STREAM), 
	//							and the TCP/IP protocol (IPPROTO_TCP).
	SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	// Check for errors to ensure that the socket is a valid socket.
	// Error detection is a key part of successful networking code. 
	// If the socket call fails, it returns INVALID_SOCKET. 
	// The if statement in the previous code is used to catch any errors that
	// may have occurred while creating the socket. WSAGetLastError returns an 
	// error number associated with the last error that occurred.
	if (INVALID_SOCKET == listenSocket)
	{
		cout << "Web Server: Error at socket(): " << WSAGetLastError() << endl;
		WSACleanup();
		return;
	}

	// For a server to communicate on a network, it must bind the socket to 
	// a network address.

	// Need to assemble the required data for connection in sockaddr structure.

	// Create a sockaddr_in object called serverService. 
	sockaddr_in serverService;
	// Address family (must be AF_INET - Internet address family).
	serverService.sin_family = AF_INET;
	// IP address. The sin_addr is a union (s_addr is a unsigned long 
	// (4 bytes) data type).
	// inet_addr (Iternet address) is used to convert a string (char *) 
	// into unsigned long.
	// The IP address is INADDR_ANY to accept connections on all interfaces.
	serverService.sin_addr.s_addr = INADDR_ANY;
	// IP Port. The htons (host to network - short) function converts an
	// unsigned short from host to TCP/IP network byte order 
	// (which is big-endian).
	serverService.sin_port = htons(TIME_PORT);

	// Bind the socket for client's requests.

	// The bind function establishes a connection to a specified socket.
	// The function uses the socket handler, the sockaddr structure (which
	// defines properties of the desired connection) and the length of the
	// sockaddr structure (in bytes).
	if (SOCKET_ERROR == bind(listenSocket, (SOCKADDR*)&serverService, sizeof(serverService)))
	{
		cout << "Web Server: Error at bind(): " << WSAGetLastError() << endl;
		closesocket(listenSocket);
		WSACleanup();
		return;
	}

	// Listen on the Socket for incoming connections.
	// This socket accepts only one connection (no pending connections 
	// from other clients). This sets the backlog parameter.
	if (SOCKET_ERROR == listen(listenSocket, 5))
	{
		cout << "Time Server: Error at listen(): " << WSAGetLastError() << endl;
		closesocket(listenSocket);
		WSACleanup();
		return;
	}
	addSocket(listenSocket, LISTEN);

	// Accept connections and handles them one by one.
	while (true)
	{
		// The select function determines the status of one or more sockets,
		// waiting if necessary, to perform asynchronous I/O. Use fd_sets for
		// sets of handles for reading, writing and exceptions. select gets "timeout" for waiting
		// and still performing other operations (Use NULL for blocking). Finally,
		// select returns the number of descriptors which are ready for use (use FD_ISSET
		// macro to check which descriptor in each set is ready to be used).
		fd_set waitRecv;
		FD_ZERO(&waitRecv);
		for (int i = 0; i < MAX_SOCKETS; i++)
		{
			if ((sockets[i].recv == LISTEN) || (sockets[i].recv == RECEIVE) && !sockets[i].responsePending)
				FD_SET(sockets[i].id, &waitRecv);
		}

		fd_set waitSend;
		FD_ZERO(&waitSend);
		for (int i = 0; i < MAX_SOCKETS; i++)
		{
			if (sockets[i].send == SEND && sockets[i].responsePending)
				FD_SET(sockets[i].id, &waitSend);
		}

		//
		// Wait for interesting event.
		// Note: First argument is ignored. The fourth is for exceptions.
		// And as written above the last is a timeout, hence we are blocked if nothing happens.
		//
		int nfd;
		nfd = select(0, &waitRecv, &waitSend, NULL, &MAX_TIME_FOR_SELECT);
		if (nfd == SOCKET_ERROR)
		{
			cout << "Web Server: Error at select(): " << WSAGetLastError() << endl;
			WSACleanup();
			return;
		}

		for (int i = 0; i < MAX_SOCKETS && nfd > 0; i++)
		{
			if (FD_ISSET(sockets[i].id, &waitRecv))
			{
				nfd--;
				switch (sockets[i].recv)
				{
				case LISTEN:
					acceptConnection(i);
					break;

				case RECEIVE:
					receiveMessage(i);
					break;
				}
			}
		}

		for (int i = 0; i < MAX_SOCKETS && nfd > 0; i++)
		{
			if (FD_ISSET(sockets[i].id, &waitSend))
			{
				nfd--;
				switch (sockets[i].send)
				{
				case SEND:
					sendMessage(i);
					break;
				}
			}
		}

		if (socketsCount >= 2)
		{
			int socketsLeft = socketsCount - 1;
			time_t currTime;
			time(&currTime);
			time_t timeToClose = difftime(currTime, TIME_FOR_TIMEOUT);
			for (int i = 1; i < MAX_SOCKETS && socketsLeft != 0; i++)
			{
				if (sockets[i].id != 0 && sockets[i].lastTimeUsed <= timeToClose && sockets[i].lastTimeUsed != 0)
				{
					closesocket(sockets[i].id);
					removeSocket(i);
					socketsLeft -= 1;
				}
				else if (sockets[i].id != 0)
					socketsLeft -= 1;
			}
		}
	}

	// Closing connections and Winsock.
	cout << "Web Server: Closing Connection.\n";
	closesocket(listenSocket);
	WSACleanup();
}

bool addSocket(SOCKET id, int what)
{
	for (int i = 0; i < MAX_SOCKETS; i++)
	{
		if (sockets[i].recv == EMPTY)
		{
			sockets[i].id = id;
			sockets[i].recv = what;
			sockets[i].send = IDLE;
			socketsCount++;
			return (true);
		}
	}
	return (false);
}

void removeSocket(int index)
{
	sockets[index].recv = EMPTY;
	sockets[index].send = EMPTY;
	socketsCount--;
}

void acceptConnection(int index)
{
	SOCKET id = sockets[index].id;
	struct sockaddr_in from;		// Address of sending partner
	int fromLen = sizeof(from);

	SOCKET msgSocket = accept(id, (struct sockaddr*)&from, &fromLen);
	if (INVALID_SOCKET == msgSocket)
	{
		cout << "Web Server: Error at accept(): " << WSAGetLastError() << endl;
		return;
	}
	cout << "Web Server: Client " << inet_ntoa(from.sin_addr) << ":" << ntohs(from.sin_port) << " is connected." << endl;

	//
	// Set the socket to be in non-blocking mode.
	//
	unsigned long flag = 1;
	if (ioctlsocket(msgSocket, FIONBIO, &flag) != 0)
	{
		cout << "Web Server: Error at ioctlsocket(): " << WSAGetLastError() << endl;
	}

	if (addSocket(msgSocket, RECEIVE) == false)
	{
		cout << "\t\tToo many connections, dropped!\n";
		closesocket(id);
	}
	return;
}


int readHeader(SocketState& state) {
	char ch;
	int bytesReceived;
	int totalBytesReceived = 0;

	while (!state.headerComplete) {
		bytesReceived = recv(state.id, &ch, 1, 0);
		if (bytesReceived == SOCKET_ERROR) {
			std::cerr << "recv failed: " << WSAGetLastError() << std::endl;
			return -1;
		}
		else if (bytesReceived == 0) {
			return 0;
		}

		totalBytesReceived += bytesReceived;
		state.header += ch;

		if (state.header.size() >= 4 && state.header.substr(state.header.size() - 4) == lineSuffix + lineSuffix) {
			state.headerComplete = true;
		}
	}

	return totalBytesReceived;
}

int extractContentLength(SocketState& state) {
	size_t contentLengthPos = state.header.find("Content-Length: ");
	if (contentLengthPos != std::string::npos) {
		contentLengthPos += 16;
		size_t endPos = state.header.find(lineSuffix, contentLengthPos);
		std::string contentLengthStr = state.header.substr(contentLengthPos, endPos - contentLengthPos);
		state.contentLength = std::stoi(contentLengthStr);
		return state.contentLength;
	}
	else {
		state.contentLength = 0;  // No content length header found
		return 0;
	}
}

int readBody(SocketState& state) {
	int totalBytesReceived = 0;
	int bytesReceived;

	while (state.body.length() < static_cast<size_t>(state.contentLength)) {
		char buffer[1024];
		int toRead = min(state.contentLength - static_cast<int>(state.body.length()), static_cast<int>(sizeof(buffer)));
		bytesReceived = recv(state.id, buffer, toRead, 0);
		if (bytesReceived == SOCKET_ERROR) {
			std::cerr << "recv failed: " << WSAGetLastError() << std::endl;
			return -1;
		}
		else if (bytesReceived == 0) {
			return 0;
		}

		totalBytesReceived += bytesReceived;
		state.body.append(buffer, bytesReceived);
	}

	return totalBytesReceived;
}

int receiveOneMessage(SocketState& state) {
	int totalBytesReceived = 0;

	if (!state.headerComplete) {
		int headerBytes = readHeader(state);
		if (headerBytes <= 0) {
			return headerBytes;
		}
		totalBytesReceived += headerBytes;

		int contentLength = extractContentLength(state);
		if (contentLength < 0) {
			return -1;
		}
	}

	if (state.contentLength > 0) {
		int bodyBytes = readBody(state);
		if (bodyBytes < 0) {
			return bodyBytes;
		}
		totalBytesReceived += bodyBytes;
	}

	state.buffer = state.header + state.body;
	time_t currectTime;
	time(&currectTime);
	state.lastTimeUsed = currectTime;

	return totalBytesReceived;
}


void receiveMessage(int index) {
	SOCKET msgSocket = sockets[index].id;

	int totalBytes = receiveOneMessage(sockets[index]);
	if (totalBytes == -1) {
		std::cout << "HTTP Server: Error at recv(): " << WSAGetLastError() << std::endl;
		closesocket(msgSocket);
		removeSocket(index);
		return;
	}
	if (totalBytes == 0) {
		closesocket(msgSocket);
		removeSocket(index);
		return;
	}
	else {
		std::cout << "HTTP Server: Received: " << totalBytes << " bytes of \"" << sockets[index].buffer << "\" message.\n";

		if (sockets[index].buffer.length() > 0) {
			if (strncmp(sockets[index].buffer.c_str(), "OPTIONS", 7) == 0) {
				sockets[index].send = SEND;
				sockets[index].method = HTTPRequestTypes::OPTIONS;
			}
			else if (strncmp(sockets[index].buffer.c_str(), "GET", 3) == 0) {
				sockets[index].send = SEND;
				sockets[index].method = HTTPRequestTypes::GET;
			}
			else if (strncmp(sockets[index].buffer.c_str(), "HEAD", 4) == 0) {
				sockets[index].send = SEND;
				sockets[index].method = HTTPRequestTypes::HEAD;
			}
			else if (strncmp(sockets[index].buffer.c_str(), "POST", 4) == 0) {
				sockets[index].send = SEND;
				sockets[index].method = HTTPRequestTypes::POST;
			}
			else if (strncmp(sockets[index].buffer.c_str(), "PUT", 3) == 0) {
				sockets[index].send = SEND;
				sockets[index].method = HTTPRequestTypes::PUT;
			}
			else if (strncmp(sockets[index].buffer.c_str(), "DELETE", 6) == 0) {
				sockets[index].send = SEND;
				sockets[index].method = HTTPRequestTypes::DELETE_REQUEST;
			}
			else if (strncmp(sockets[index].buffer.c_str(), "TRACE", 5) == 0) {
				sockets[index].send = SEND;
				sockets[index].method = HTTPRequestTypes::TRACE;
			}
			else {
				sockets[index].send = SEND;
				sockets[index].method = HTTPRequestTypes::NOT_ALLOWED;
			}
		}
	}

	sockets[index].responsePending = true;
}



void sendMessage(int index) {
	int bytesSent = 0;
	char sendBuff[512];

	SOCKET msgSocket = sockets[index].id;
	if (sockets[index].method == HTTPRequestTypes::OPTIONS) {
		optionsCommand(sendBuff, bytesSent, sockets[index]);
	}
	else if (sockets[index].method == HTTPRequestTypes::GET) {
		getCommand(sendBuff, bytesSent, sockets[index]);
	}
	else if (sockets[index].method == HTTPRequestTypes::HEAD) {
		headCommand(sendBuff, bytesSent, sockets[index]);
	}
	else if (sockets[index].method == HTTPRequestTypes::POST) {
		postCommand(sendBuff, bytesSent, sockets[index]);
	}
	else if (sockets[index].method == HTTPRequestTypes::PUT) {
		putCommand(sendBuff, bytesSent, sockets[index]);
	}
	else if (sockets[index].method == HTTPRequestTypes::DELETE_REQUEST) {
		deleteCommand(sendBuff, bytesSent, sockets[index]);
	}
	else if (sockets[index].method == HTTPRequestTypes::TRACE) {
		traceCommand(sendBuff, bytesSent, sockets[index]);
	}
	else if (sockets[index].method == HTTPRequestTypes::NOT_ALLOWED) {
		closesocket(msgSocket);
		removeSocket(index);
		return;
	}

	bytesSent = send(msgSocket, sendBuff, (int)strlen(sendBuff), 0);
	if (SOCKET_ERROR == bytesSent) {
		std::cout << "Web Server: Error at send(): " << WSAGetLastError() << std::endl;
		return;
	}

	sockets[index].body = "";
	sockets[index].buffer = "";
	sockets[index].contentLength = 0;
	sockets[index].header = "";
	sockets[index].headerComplete = false;
	sockets[index].responsePending = false;


	std::cout << "Web Server: Sent: " << bytesSent << "\\" << strlen(sendBuff) << " bytes of \"" << sendBuff << "\" message.\n";

	time_t currectTime;
	time(&currectTime);
	sockets[index].lastTimeUsed = currectTime;
	sockets[index].send = IDLE;
}


