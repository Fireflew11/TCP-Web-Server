#ifndef HTTP_H
#define HTTP_H

#include <string>
#include "server.h"

using namespace std;

void makeHeader(string& response, string status, string contentType);
void makeBody(string& response, string body);
void GetMethodHandler(string& response, string& socketBuffer, const char* queryString = nullptr);
void HeadMethodHandler(string& response, string& socketBuffer);
void OptionsMethodHandler(string& response);
void NotAllowMethodHandler(string& response);
void TraceMethodHandler(string& response, string& socketBuffer);
void PutMethodHandler(string& response, const char* body);
void PostMethodHandler(string& response, const char* body);
void DeleteMethodHandler(string& response, string& socketBuffer);
string makePostBody(const string& body);
string escapeHTML(const string& input);

// Function declarations for HTTP commands
void optionsCommand(char* sendBuff, int& bytesSent, const char* buffer);
void getCommand(char* sendBuff, int& bytesSent, const char* buffer);
void headCommand(char* sendBuff, int& bytesSent, const char* buffer);
void postCommand(char* sendBuff, int& bytesSent, const char* buffer);
void putCommand(char* sendBuff, int& bytesSent, const char* buffer);
void deleteCommand(char* sendBuff, int& bytesSent, const char* buffer);
void traceCommand(char* sendBuff, int& bytesSent, const char* buffer);

#endif // HTTP_H
