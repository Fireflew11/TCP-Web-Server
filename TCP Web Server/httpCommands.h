#pragma once
#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

//#pragma comment(lib, "Ws2_32.lib")
#include <iostream>
#include <winsock2.h>
#include <string>
#include <unordered_map>
#include <fstream>
#include <filesystem>
#include <time.h>
#include <ctime>
#include <sstream>

using namespace std;


const int SOCKET_BUFFER_SIZE = 2048;
const std::string ROOT_DIRECTORY = "C://temp/"; // default working directory
const std::string DEFAULT_RESOURCE = "/index.html"; // default index

enum HTTPRequestTypes {
	OPTIONS = 1,
	GET = 2,
	HEAD = 3,
	POST = 4,
	PUT = 5,
	DELETE_REQUEST = 6,
	TRACE = 7,
};

struct SocketState
{
	SOCKET id;			// Socket handle
	int	recv;			// Receiving?
	int	send;			// Sending?
	HTTPRequestTypes sendSubType;	// Sending sub-type
	char buffer[512];
	int len;
	std::time_t lastTimeUsed;
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

enum LANG {
	en,
	he,
	fr
};

void makeHeader(string& response, string status, string contentType);
void makeBody(string& response, string body);
void GetMethodHandler(string& response, string& socketBuffer, const char* queryString);
void HeadMethodHandler(string& response, string& socketBuffer);
void OptionsMethodHandler(string& response);
void NotAllowMethodHandler(string& response);
void TraceMethodHandler(string& response, string& socketBuffer);
void PutMethodHandler(string& response, string& socketBuffer);
string makePostBody(const string& body);
void PostMethodHandler(string& response, string& socketBuffer, const char* body);
void DeleteMethodHandler(string& response, string& socketBuffer);
void getCommand(char* sendBuff, int& bytesSent, SocketState curSocket);
void headCommand(char* sendBuff, int& bytesSent);
void postCommand(char* sendBuff, int& bytesSent, const char* body);
void optionsCommand(char* sendBuff, int& bytesSent);
void putCommand(char* sendBuff, int& bytesSent);
void deleteCommand(char* sendBuff, int& bytesSent);
void traceCommand(char* sendBuff, int& bytesSent);
void makeHeader(string& response, string status, string contentType);
bool addSocket(SOCKET id, int what);
void removeSocket(int index);
void acceptConnection(int index);
void receiveMessage(int index);
void sendMessage(int index);
void optionsCommand(char* sendBuff, int& bytesSent);
void getCommand(char* sendBuff, int& bytesSent, SocketState curSocket);
void headCommand(char* sendBuff, int& bytesSent);
void postCommand(char* sendBuff, int& bytesSent, const char* body);
void putCommand(char* sendBuff, int& bytesSent);
void deleteCommand(char* sendBuff, int& bytesSent);
void traceCommand(char* sendBuff, int& bytesSent);