#pragma once
#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <iostream>
#include <winsock2.h>
#include <windows.h>
#include <string>
#include <unordered_map>
#include <fstream>
#include <direct.h> // For directory operations
#include <ctime>
#include <sstream>


using namespace std;

const int SOCKET_BUFFER_SIZE = 2048;
const std::string DEFAULT_RESOURCE = "\\index.html"; // default index
const std::string RESOURCE_PATH = "\\Resources"; // folder to create new files in for POST, change with PUT, and delete with DELETE
const std::string lineSuffix = "\r\n";

enum HTTPRequestTypes {
    OPTIONS = 1,
    GET = 2,
    HEAD = 3,
    POST = 4,
    PUT = 5,
    DELETE_REQUEST = 6,
    TRACE = 7,
    NOT_ALLOWED = 8
};

struct SocketState {
    SOCKET id;              // Socket handle
    int recv;               // Receiving?
    int send;               // Sending?
    HTTPRequestTypes method; // METHOD
    std::string buffer;     // Buffer for received data
    std::time_t lastTimeUsed;
    std::string header;     // Parsed header
    std::string body;       // Parsed body
    int contentLength;      // Content length of the body
    bool headerComplete;    // Flag to check if header is complete
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

void makeHeader(string& response, string status, string contentType);
void makeBody(string& response, string body);
void GetMethodHandler(string& response, const SocketState& state);
void HeadMethodHandler(string& response, const SocketState& state);
void OptionsMethodHandler(string& response, const SocketState& state);
void NotAllowMethodHandler(string& response, const SocketState& state);
void TraceMethodHandler(string& response, const SocketState& state);
void PutMethodHandler(string& response, const SocketState& state);
string makePostBody(const string& body);
void PostMethodHandler(string& response, const SocketState& state);
void DeleteMethodHandler(string& response, const SocketState& state);

void getCommand(char* sendBuff, int& bytesSent, const SocketState& state);
void headCommand(char* sendBuff, int& bytesSent, const SocketState& state);
void postCommand(char* sendBuff, int& bytesSent, const SocketState& state);
void optionsCommand(char* sendBuff, int& bytesSent, const SocketState& state);
void putCommand(char* sendBuff, int& bytesSent, const SocketState& state);
void deleteCommand(char* sendBuff, int& bytesSent, const SocketState& state);
void traceCommand(char* sendBuff, int& bytesSent, const SocketState& state);

const string getLanguageFromQuery(const string& buffer);
const string getRequestBody(const string& buffer);
void initializeBaseDirectory();
