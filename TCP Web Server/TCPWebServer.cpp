#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <iostream>
using namespace std;
#pragma comment(lib, "Ws2_32.lib")
#include <winsock2.h>
#include <string.h>
#include <time.h>
#include <ctime>
#include <unordered_map>
#include <sstream>
#include <algorithm>
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
const string lineSuffix = "\r\n";

bool addSocket(SOCKET id, int what);
void removeSocket(int index);
void acceptConnection(int index);
void receiveMessage(int index);
void sendMessage(int index);
void optionsCommand(char* sendBuff, int& bytesSent);
void getCommand(char* sendBuff, int& bytesSent);
void headCommand(char* sendBuff, int& bytesSent);
void postCommand(char* sendBuff, int& bytesSent, const char* body);
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
		cout << "Web Server: Error at recv(): " << WSAGetLastError() << endl;
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
		sockets[index].buffer[len + bytesRecv] = '\0'; //add the null-terminating to make it a string
		cout << "Web Server: Recieved: " << bytesRecv << " bytes of \"" << &sockets[index].buffer[len] << "\" message.\n";

		sockets[index].len += bytesRecv;

		if (sockets[index].len > 0)
		{
			if (strncmp(sockets[index].buffer, "OPTIONS", 7) == 0)
			{
				sockets[index].send = SEND;
				sockets[index].sendSubType = HTTPRequestTypes::OPTIONS;
				memcpy(sockets[index].buffer, &sockets[index].buffer[7], sockets[index].len - 7);
				sockets[index].len -= 7;
				return;
			}
			else if (strncmp(sockets[index].buffer, "GET", 3) == 0)
			{
				sockets[index].send = SEND;
				sockets[index].sendSubType = HTTPRequestTypes::GET;
				memcpy(sockets[index].buffer, &sockets[index].buffer[3], sockets[index].len - 3);
				sockets[index].len -= 3;
				return;
			}
			else if (strncmp(sockets[index].buffer, "HEAD", 4) == 0)
			{
				sockets[index].send = SEND;
				sockets[index].sendSubType = HTTPRequestTypes::HEAD;
				memcpy(sockets[index].buffer, &sockets[index].buffer[4], sockets[index].len - 4);
				sockets[index].len -= 4;
				return;
			}
			else if (strncmp(sockets[index].buffer, "POST", 4) == 0)
			{
				sockets[index].send = SEND;
				sockets[index].sendSubType = HTTPRequestTypes::POST;
				memcpy(sockets[index].buffer, &sockets[index].buffer[4], sockets[index].len - 4);
				sockets[index].len -= 4;
				return;
			}
			else if (strncmp(sockets[index].buffer, "PUT", 3) == 0)
			{
				sockets[index].send = SEND;
				sockets[index].sendSubType = HTTPRequestTypes::PUT;
				memcpy(sockets[index].buffer, &sockets[index].buffer[3], sockets[index].len - 3);
				sockets[index].len -= 3;
				return;
			}
			else if (strncmp(sockets[index].buffer, "DELETE", 6) == 0)
			{
				sockets[index].send = SEND;
				sockets[index].sendSubType = HTTPRequestTypes::DELETE;
				memcpy(sockets[index].buffer, &sockets[index].buffer[6], sockets[index].len - 6);
				sockets[index].len -= 6;
				return;
			}
			else if (strncmp(sockets[index].buffer, "TRACE", 5) == 0)
			{
				sockets[index].send = SEND;
				sockets[index].sendSubType = HTTPRequestTypes::TRACE;
				memcpy(sockets[index].buffer, &sockets[index].buffer[5], sockets[index].len - 5);
				sockets[index].len -= 5;
				return;
			}
			else if (strncmp(sockets[index].buffer, "Exit", 4) == 0)
			{
				closesocket(msgSocket);
				removeSocket(index);
				return;
			}
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
		postCommand(sendBuff, bytesSent, sockets[index].buffer);//הוספתי את תוכן הבקשה כפרמטר שאותו השרת ידפיס
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


// פונקציה שיוצרת כותרת לתגובה HTTP
void makeHeader(string& response, string status, string contentType) {

	response = "HTTP/1.1 " + status + lineSuffix;
	response += "Server: HTTP Web Server" + lineSuffix;
	response += "Content-Type: " + contentType + lineSuffix;
}

// פונקציה שיוצרת גוף לתגובה HTTP
void makeBody(string& response, string body) {
	response += "Content-Length: " + to_string(body.length()) + lineSuffix + lineSuffix;
	response += body;
}


void GetMethodHandler(string& response, string& socketBuffer, const char* queryString = nullptr) {
	string status = "200 OK";
	string resourcePath = "/index.html"; 
	string contentType = "text/html";
	string HTMLContent;

	// Check if query string contains "lang" parameter
	if (queryString != nullptr) {
		const char* langParam = strstr(queryString, "lang=");
		if (langParam != nullptr) {
			langParam += 5; // Move to the beginning of language code
			if (strncmp(langParam, "he", 2) == 0)
				HTMLContent = "<html><body><h1>!שלום עולם</h1></body></html>";
			else if (strncmp(langParam, "fr", 2) == 0)
				HTMLContent = "<html><body><h1>Bonjour le monde!</h1></body></html>";
			else //en + default
				HTMLContent = "<html><body><h1>Hello World!</h1></body></html>";
		}
	}
	//אם הדיפולט לא עובד
	//if (HTMLContent.empty())// If language parameter not provided or invalid, default to English
	//	HTMLContent = "<html><body><h1>Hello, World!</h1></body></html>";

	makeHeader(response, status, contentType);
	makeBody(response, HTMLContent);
}

// פונקציה שמטפלת בבקשת HEAD
void HeadMethodHandler(string& response, string& socketBuffer) { // כעיקרון האד לא מחזירה בודי, צריך להבין מה לעשות פה
	GetMethodHandler(response, socketBuffer);
	//HTTP / 1.1 200 OK // התגובה אמורה להראות ככה 
 //   Content - Type: text / html
		
}

// פונקציה שמטפלת בבקשת OPTIONS
void OptionsMethodHandler(string& response) {
	string status = "204 no content";
	string contentType = "text/html";

	makeHeader(response, status, contentType);
	response += "Supported methods: OPTIONS, GET, HEAD, POST, PUT, DELETE, TRACE" + lineSuffix;
}

// פונקציה שמטפלת בבקשות שאינן מותרות
void NotAllowMethodHandler(string& response) {// לבדוק אם צריך
	string status = "405 Method Not Allowed";
	string contentType = "text/html";

	makeHeader(response, status, contentType);
	response += "Allow: OPTIONS, GET, HEAD, POST, PUT, DELETE, TRACE" + lineSuffix;
	response += "Connection: close" + lineSuffix + lineSuffix;
}

// פונקציה שמטפלת בבקשת TRACE
void TraceMethodHandler(string& response, string& socketBuffer) {
	string status = "200 OK";
	string contentType = "message/http";
	string traceString = "TRACE " + socketBuffer;

	makeHeader(response, status, contentType);
	makeBody(response, traceString);
}

// פונקציה שמטפלת בבקשת PUT
void PutMethodHandler(string& response, string& socketBuffer) {
	string status = "201 Created";
	string contentType = "text/html";
	string body = socketBuffer;

	makeHeader(response, status, contentType);
	makeBody(response, body);
}
string makePostBody(const string& body)
{
	ostringstream htmlBody;

	htmlBody << "<!DOCTYPE html>\n";
	htmlBody << "<html>\n" << "<head>\n";
	htmlBody << "<title>Post Method</title>\n";
	htmlBody << "</head>\n" << "<body>\n";
	htmlBody << "<h1>POST Succeded</h1>\n";
	htmlBody << "<p>The Content is \"<a>" << body << " \"</a>.</p>\n";
	htmlBody << "</body>\n" << "</html>\n";

	return htmlBody.str();
}

// פונקציה שמטפלת בבקשת POST
void PostMethodHandler(string& response, string& socketBuffer, const char* body) {
	string status = "201 Created";
	string contentType = "text/html";
	string postBody = makePostBody(body); //אמור להכניס לבודי את תוכן הבקשה - צריכים להניח שזה סטרינג
	
	makeHeader(response, status, contentType);
	makeBody(response, postBody);
}


// פונקציה שמטפלת בבקשת DELETE
void DeleteMethodHandler(string& response, string& socketBuffer) {
	string status = "200 OK";
	string contentType = "text/html";
	string body = "<html><body><h1>Resource Deleted Successfully</h1></body></html>";

	makeHeader(response, status, contentType);
	makeBody(response, body);
}

void getCommand(char* sendBuff, int& bytesSent) {
	string response;
	string socketBuffer = "GET /index.html HTTP/1.1\r\nHost: localhost\r\nAccept: text/html\r\n\r\n";
	GetMethodHandler(response, socketBuffer);
	bytesSent = response.size();
	strcpy(sendBuff, response.c_str());
}

void headCommand(char* sendBuff, int& bytesSent) {
	string response;
	string socketBuffer = "HEAD /index.html HTTP/1.1\r\nHost: localhost\r\nAccept: text/html\r\n\r\n";
	HeadMethodHandler(response, socketBuffer);
	bytesSent = response.size();
	strcpy(sendBuff, response.c_str());
}

void postCommand(char* sendBuff, int& bytesSent, const char* body) {
	string response;
	string Buffer = "POST /create HTTP/1.1\r\nHost: localhost\r\nAccept: text/html\r\n\r\nTest file content";
	PostMethodHandler(response, Buffer, body);
	bytesSent = response.size();
	strcpy(sendBuff, response.c_str());
}

void optionsCommand(char* sendBuff, int& bytesSent) {
	string response;
	OptionsMethodHandler(response); 
	bytesSent = response.size();
	strcpy(sendBuff, response.c_str());
	
}

void putCommand(char* sendBuff, int& bytesSent) {
	string response;
	string socketBuffer = "PUT /create HTTP/1.1\r\nHost: localhost\r\nAccept: text/htm\r\n\r\nTest file content";
	PutMethodHandler(response, socketBuffer);
	bytesSent = response.size();
	strcpy(sendBuff, response.c_str());
}

void deleteCommand(char* sendBuff, int& bytesSent) {
	string response;
	string socketBuffer = "DELETE /resource HTTP/1.1\r\nHost: localhost\r\nAccept: text/html\r\n\r\n";
	DeleteMethodHandler(response, socketBuffer);
	bytesSent = response.size();
	strcpy(sendBuff, response.c_str());
}

void traceCommand(char* sendBuff, int& bytesSent) {
	string response;
	string socketBuffer = "TRACE /index.html HTTP/1.1\r\nHost: localhost\r\nAccept: text/html\r\n\r\n";
	TraceMethodHandler(response, socketBuffer);
	bytesSent = response.size();
	strcpy(sendBuff, response.c_str());
}



//void optionsCommand(char* sendBuff, int& bytesSent) { // מבקשת מידע על הפעולות האפשריות על האובייקט
//	const char* response = "<html><body><h1>Supported methods: OPTIONS, GET, HEAD, POST, PUT, DELETE, TRACE</h1></body></html>";
//	bytesSent = strlen(response);
//	memcpy(sendBuff, response, bytesSent);
//}
//
//
//void getCommand(char* sendBuff, int& bytesSent, const char* queryString) { // קוראת את האובייקט
//	const char* lang = "en"; // Default language
//	const char* content = "Hello World!"; // Default content
//
//	if (queryString != nullptr) {
//		const char* langParam = strstr(queryString, "lang=");
//		if (langParam != nullptr) {
//			langParam += 5; // Move to the beginning of language code
//			if (strncmp(langParam, "he", 2) == 0)
//				lang = "he";
//			else if (strncmp(langParam, "fr", 2) == 0)
//				lang = "fr";
//		}
//	}
//
//	if (strcmp(lang, "he") == 0)
//		content = "שלום עולם";
//	else if (strcmp(lang, "fr") == 0)
//		content = "Bonjour le monde!";
//
//	const char* responseTemplate = "<html><body><h1>%s</h1></body></html>";//need to check if %s works
//	int contentLength = strlen(content) + strlen(responseTemplate) - 2;
//	char* response = new char[contentLength];
//	snprintf(response, contentLength, responseTemplate, content); //מחבר את הכל
//	bytesSent = strlen(response);
//	memcpy(sendBuff, response, bytesSent);
//	delete[] response;//לא בטוח
//}
//
//void headCommand(char* sendBuff, int& bytesSent) {// Implementation for HEAD command
//	const char* response = "<html><body>Response for HEAD command</body></html>";
//	bytesSent = strlen(response);
//	memcpy(sendBuff, response, bytesSent);
//}
//
//void postCommand(char* sendBuff, int& bytesSent, const char* body) {
//	std::cout << "Received POST data: " << body << std::endl;
//	const char* responseTemplate =
//		"HTTP/1.1 200 OK\r\n"
//		"Content-Type: text/html\r\n"
//		"Content-Length: %d\r\n"
//		"\r\n"
//		"<html><body>Received POST data: %s</body></html>";
//	int contentLength = strlen(body) + strlen("<html><body>Received POST data: </body></html>");
//	sprintf(sendBuff, responseTemplate, contentLength, body);
//	bytesSent = strlen(sendBuff);
//}
//
//void putCommand(char* sendBuff, int& bytesSent) {
//	const char* response =
//		"HTTP/1.1 200 OK\r\n"
//		"Content-Type: text/html\r\n"
//		"Content-Length: 24\r\n"
//		"\r\n"
//		"<html><body>OK</body></html>";
//	strcpy(sendBuff, response);
//	bytesSent = strlen(response);
//}
//
//void deleteCommand(char* sendBuff, int& bytesSent) {
//	const char* response =
//		"HTTP/1.1 200 OK\r\n"
//		"Content-Type: text/html\r\n"
//		"Content-Length: 24\r\n"
//		"\r\n"
//		"<html><body>OK</body></html>";
//	strcpy(sendBuff, response);
//	bytesSent = strlen(response);
//}
//
//void traceCommand(char* sendBuff, int& bytesSent, const char* receivedMessage) {
//	const char* responseTemplate =
//		"HTTP/1.1 200 OK\r\n"
//		"Content-Type: text/html\r\n"
//		"Content-Length: %d\r\n"
//		"\r\n"
//		"<html><body>%s</body></html>";
//	int contentLength = strlen(receivedMessage) + strlen("<html><body></body></html>");
//	sprintf(sendBuff, responseTemplate, contentLength, receivedMessage);
//	bytesSent = strlen(sendBuff);
//}