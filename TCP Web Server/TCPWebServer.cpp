#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
using namespace std;
#pragma comment(lib, "Ws2_32.lib")
#include <winsock2.h>
#include <string.h>
#include <time.h>
#ifdef DELETE
#undef DELETE
#endif // DELETE

enum HTTPRequestTypes {
	OPTIONS = 1,
	GET = 2,
	HEAD = 3,
	POST = 4,
	PUT = 5,
	DELETE = 6,
	TRACE = 7,

};

struct SocketState
{
	SOCKET id;			// Socket handle
	int	recv;			// Receiving?
	int	send;			// Sending?
	HTTPRequestTypes sendSubType;	// Sending sub-type
	char buffer[128];
	int len;
};

const int TIME_PORT = 27015;
const int MAX_SOCKETS = 60;
const int EMPTY = 0;
const int LISTEN = 1;
const int RECEIVE = 2;
const int IDLE = 3;
const int SEND = 4;
const int SEND_TIME = 1;
const int SEND_SECONDS = 2;

bool addSocket(SOCKET id, int what);
void removeSocket(int index);
void acceptConnection(int index);
void receiveMessage(int index);
void sendMessage(int index);
void optionsCommand(char* sendBuff, int& bytesSent);
void getCommand(char* sendBuff, int& bytesSent);
void headCommand(char* sendBuff, int& bytesSent);
void postCommand(char* sendBuff, int& bytesSent);
void putCommand(char* sendBuff, int& bytesSent);
void deleteCommand(char* sendBuff, int& bytesSent);
void traceCommand(char* sendBuff, int& bytesSent);

struct SocketState sockets[MAX_SOCKETS] = { 0 };
int socketsCount = 0;


void main()
{
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
			if ((sockets[i].recv == LISTEN) || (sockets[i].recv == RECEIVE))
				FD_SET(sockets[i].id, &waitRecv);
		}

		fd_set waitSend;
		FD_ZERO(&waitSend);
		for (int i = 0; i < MAX_SOCKETS; i++)
		{
			if (sockets[i].send == SEND)
				FD_SET(sockets[i].id, &waitSend);
		}

		//
		// Wait for interesting event.
		// Note: First argument is ignored. The fourth is for exceptions.
		// And as written above the last is a timeout, hence we are blocked if nothing happens.
		//
		int nfd;
		nfd = select(0, &waitRecv, &waitSend, NULL, NULL);
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
			sockets[i].len = 0;
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

void receiveMessage(int index)
{
	SOCKET msgSocket = sockets[index].id;

	int len = sockets[index].len;
	int bytesRecv = recv(msgSocket, &sockets[index].buffer[len], sizeof(sockets[index].buffer) - len, 0);

	if (SOCKET_ERROR == bytesRecv)
	{
		std::cout << "Web Server: Error at recv(): " << WSAGetLastError() << std::endl;
		closesocket(msgSocket);
		removeSocket(index);
		return;
	}
	if (bytesRecv == 0)
	{
		closesocket(msgSocket);
		removeSocket(index);
		return;
	}
	else
	{
		sockets[index].buffer[len + bytesRecv] = '\0'; // Add the null-terminating to make it a string
		std::cout << "Web Server: Received: " << bytesRecv << " bytes of \"" << &sockets[index].buffer[len] << "\" message.\n";

		sockets[index].len += bytesRecv;

		// Extract the request line
		char* requestLine = strtok(sockets[index].buffer, "\r\n");
		if (requestLine != nullptr)
		{
			char method[8];
			char url[128];
			char version[16];

			// Parse the request line
			sscanf(requestLine, "%s %s %s", method, url, version);

			std::cout << "Web Server: Method: " << method << ", URL: " << url << ", Version: " << version << std::endl;

			// Check the method and set the sendSubType accordingly
			if (strcmp(method, "OPTIONS") == 0)
			{
				sockets[index].send = SEND;
				sockets[index].sendSubType = HTTPRequestTypes::OPTIONS;
			}
			else if (strcmp(method, "GET") == 0)
			{
				sockets[index].send = SEND;
				sockets[index].sendSubType = HTTPRequestTypes::GET;

				// Extract the query string from the URL
				char* queryString = strchr(url, '?');
				if (queryString != nullptr)
				{
					*queryString = '\0'; // Terminate URL at '?'
					queryString++; // Move to the query string part

					// Parse the query string to find the "lang" parameter
					char* token = strtok(queryString, "&");
					char lang[32] = "";
					while (token != nullptr)
					{
						if (strncmp(token, "lang=", 5) == 0)
						{
							strcpy(lang, token + 5);
							break;
						}
						token = strtok(nullptr, "&");
					}

					std::cout << "Web Server: Path: " << url << ", Query: " << queryString << ", Lang: " << lang << std::endl;

					// Pass the lang parameter to the getCommand method
					sockets[index].len = sprintf(sockets[index].buffer, "%s", lang);
				}
				else
				{
					std::cout << "Web Server: Path: " << url << ", No Query" << std::endl;
					sockets[index].buffer[0] = '\0'; // No lang parameter
					sockets[index].len = 0;
				}
			}
			else if (strcmp(method, "HEAD") == 0)
			{
				sockets[index].send = SEND;
				sockets[index].sendSubType = HTTPRequestTypes::HEAD;
			}
			else if (strcmp(method, "POST") == 0)
			{
				sockets[index].send = SEND;
				sockets[index].sendSubType = HTTPRequestTypes::POST;
			}
			else if (strcmp(method, "PUT") == 0)
			{
				sockets[index].send = SEND;
				sockets[index].sendSubType = HTTPRequestTypes::PUT;
			}
			else if (strcmp(method, "DELETE") == 0)
			{
				sockets[index].send = SEND;
				sockets[index].sendSubType = HTTPRequestTypes::DELETE;
			}
			else if (strcmp(method, "TRACE") == 0)
			{
				sockets[index].send = SEND;
				sockets[index].sendSubType = HTTPRequestTypes::TRACE;
			}
			else if (strcmp(method, "Exit") == 0)
			{
				closesocket(msgSocket);
				removeSocket(index);
				return;
			}

			// Save the remaining part of the buffer
			int remainingDataLength = sockets[index].len - (requestLine - sockets[index].buffer) - strlen(requestLine) - 2;
			memmove(sockets[index].buffer, requestLine + strlen(requestLine) + 2, remainingDataLength);
			sockets[index].len = remainingDataLength;
		}
	}
}


void sendMessage(int index)
{
	int bytesSent = 0;
	char sendBuff[255];

	SOCKET msgSocket = sockets[index].id;
	if (sockets[index].sendSubType == HTTPRequestTypes::OPTIONS)
	{
		optionsCommand(sendBuff, bytesSent);
	}
	else if (sockets[index].sendSubType == HTTPRequestTypes::GET) {
		getCommand(sendBuff, bytesSent);
	}
	else if (sockets[index].sendSubType == HTTPRequestTypes::HEAD) {
		headCommand(sendBuff, bytesSent);
	}
	else if (sockets[index].sendSubType == HTTPRequestTypes::POST) {
		postCommand(sendBuff, bytesSent);
	}
	else if (sockets[index].sendSubType == HTTPRequestTypes::PUT) {
		putCommand(sendBuff, bytesSent);
	}
	else if (sockets[index].sendSubType == HTTPRequestTypes::DELETE) {
		deleteCommand(sendBuff, bytesSent);
	}
	else if (sockets[index].sendSubType == HTTPRequestTypes::TRACE) {
		traceCommand(sendBuff, bytesSent);
	}

	bytesSent = send(msgSocket, sendBuff, (int)strlen(sendBuff), 0);
	if (SOCKET_ERROR == bytesSent)
	{
		cout << "Web Server: Error at send(): " << WSAGetLastError() << endl;
		return;
	}

	cout << "Web Server: Sent: " << bytesSent << "\\" << strlen(sendBuff) << " bytes of \"" << sendBuff << "\" message.\n";

	sockets[index].send = IDLE;
}