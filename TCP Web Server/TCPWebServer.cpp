/*
#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <iostream>
#include <sstream>
#include <winsock2.h>
#include <string.h>
#include <ctime>
#include <unordered_map>
#include <algorithm>

#pragma comment(lib, "Ws2_32.lib")
#ifdef DELETE
#undef DELETE
#endif // DELETE

using namespace std;

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
    SOCKET id;            // Socket handle
    int recv;            // Receiving?
    int send;            // Sending?
    HTTPRequestTypes sendSubType;    // Sending sub-type
    char buffer[512];
    int len;
    int lastIndex;       // New field to keep track of the buffer index
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
void getCommand(char* sendBuff, int& bytesSent, SocketState curSocket);
void headCommand(char* sendBuff, int& bytesSent);
void postCommand(char* sendBuff, int& bytesSent, const char* body);
void putCommand(char* sendBuff, int& bytesSent, const char* body);
void deleteCommand(char* sendBuff, int& bytesSent);
void traceCommand(char* sendBuff, int& bytesSent, SocketState socket);
string makePostBody(const string& body);

struct SocketState sockets[MAX_SOCKETS] = { 0 };
int socketsCount = 0;

void main()
{
    // Initialize Winsock (Windows Sockets).
    WSAData wsaData;

    if (NO_ERROR != WSAStartup(MAKEWORD(2, 2), &wsaData))
    {
        cout << "Web Server: Error at WSAStartup()\n";
        return;
    }

    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (INVALID_SOCKET == listenSocket)
    {
        cout << "Web Server: Error at socket(): " << WSAGetLastError() << endl;
        WSACleanup();
        return;
    }

    sockaddr_in serverService;
    serverService.sin_family = AF_INET;
    serverService.sin_addr.s_addr = INADDR_ANY;
    serverService.sin_port = htons(TIME_PORT);

    if (SOCKET_ERROR == bind(listenSocket, (SOCKADDR*)&serverService, sizeof(serverService)))
    {
        cout << "Web Server: Error at bind(): " << WSAGetLastError() << endl;
        closesocket(listenSocket);
        WSACleanup();
        return;
    }

    if (SOCKET_ERROR == listen(listenSocket, 5))
    {
        cout << "Time Server: Error at listen(): " << WSAGetLastError() << endl;
        closesocket(listenSocket);
        WSACleanup();
        return;
    }
    addSocket(listenSocket, LISTEN);

    while (true)
    {
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
            sockets[i].lastIndex = 0; // Initialize lastIndex
            socketsCount++;
            return true;
        }
    }
    return false;
}

void removeSocket(int index)
{
    sockets[index].recv = EMPTY;
    sockets[index].send = EMPTY;
    sockets[index].lastIndex = 0; // Reset lastIndex
    socketsCount--;
}

void acceptConnection(int index)
{
    SOCKET id = sockets[index].id;
    struct sockaddr_in from;
    int fromLen = sizeof(from);

    SOCKET msgSocket = accept(id, (struct sockaddr*)&from, &fromLen);
    if (INVALID_SOCKET == msgSocket)
    {
        cout << "Web Server: Error at accept(): " << WSAGetLastError() << endl;
        return;
    }
    cout << "Web Server: Client " << inet_ntoa(from.sin_addr) << ":" << ntohs(from.sin_port) << " is connected." << endl;

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

    int availableBufferSpace = sizeof(sockets[index].buffer) - sockets[index].lastIndex - 1;

    int bytesRecv = recv(msgSocket, sockets[index].buffer + sockets[index].lastIndex, availableBufferSpace, 0);

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
        sockets[index].lastIndex += bytesRecv;
        sockets[index].buffer[sockets[index].lastIndex] = '\0';

        std::cout << "Web Server: Received: " << bytesRecv << " bytes of \"" << &sockets[index].buffer[sockets[index].lastIndex - bytesRecv] << "\" message.\n";

        char* requestLineEnd = strstr(sockets[index].buffer, "\r\n");
        if (requestLineEnd != nullptr)
        {
            *requestLineEnd = '\0';
            char requestLine[256];
            strcpy(requestLine, sockets[index].buffer);
            *requestLineEnd = '\r';

            char method[8];
            char url[128];
            char version[16];

            sscanf(requestLine, "%s %s %s", method, url, version);

            std::cout << "Web Server: Method: " << method << ", URL: " << url << ", Version: " << version << std::endl;

            if (strcmp(method, "OPTIONS") == 0)
            {
                sockets[index].send = SEND;
                sockets[index].sendSubType = HTTPRequestTypes::OPTIONS;
            }
            else if (strcmp(method, "GET") == 0)
            {
                sockets[index].send = SEND;
                sockets[index].sendSubType = HTTPRequestTypes::GET;

                char* queryString = strchr(url, '?');
                char lang[32] = "en";
                if (queryString != nullptr)
                {
                    queryString++;
                    char* token = strtok(queryString, "&");
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
                }
                else
                {
                    std::cout << "Web Server: Path: " << url << ", No Query" << std::endl;
                }

                sockets[index].len = sprintf(sockets[index].buffer, "lang=%s", lang);
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

                char* body = strstr(requestLineEnd + 2, "\r\n\r\n");
                if (body != nullptr)
                {
                    body += 4;
                    std::cout << "Web Server: POST Body: " << body << std::endl;
                    strcpy(sockets[index].buffer, body);
                }
            }
            else if (strcmp(method, "PUT") == 0)
            {
                sockets[index].send = SEND;
                sockets[index].sendSubType = HTTPRequestTypes::PUT;

                char* body = strstr(requestLineEnd + 2, "\r\n\r\n");
                if (body != nullptr)
                {
                    body += 4;
                    std::cout << "Web Server: PUT Body: " << body << std::endl;
                    strcpy(sockets[index].buffer, body);
                }
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

            int remainingDataLength = sockets[index].lastIndex - (requestLineEnd - sockets[index].buffer) - 2;
            if (remainingDataLength > 0)
            {
                memmove(sockets[index].buffer, requestLineEnd + 2, remainingDataLength);
            }
            sockets[index].lastIndex = remainingDataLength;
        }
    }
}

void sendMessage(int index)
{
    int bytesSent = 0;
    char sendBuff[512];

    SOCKET msgSocket = sockets[index].id;
    if (sockets[index].sendSubType == HTTPRequestTypes::OPTIONS)
    {
        optionsCommand(sendBuff, bytesSent);
    }
    else if (sockets[index].sendSubType == HTTPRequestTypes::GET)
    {
        getCommand(sendBuff, bytesSent, sockets[index]);
    }
    else if (sockets[index].sendSubType == HTTPRequestTypes::HEAD)
    {
        headCommand(sendBuff, bytesSent);
    }
    else if (sockets[index].sendSubType == HTTPRequestTypes::POST)
    {
        postCommand(sendBuff, bytesSent, sockets[index].buffer);
    }
    else if (sockets[index].sendSubType == HTTPRequestTypes::PUT)
    {
        putCommand(sendBuff, bytesSent, sockets[index].buffer);
    }
    else if (sockets[index].sendSubType == HTTPRequestTypes::DELETE)
    {
        deleteCommand(sendBuff, bytesSent);
    }
    else if (sockets[index].sendSubType == HTTPRequestTypes::TRACE)
    {
        traceCommand(sendBuff, bytesSent, sockets[index]);
    }

    bytesSent = send(msgSocket, sendBuff, (int)strlen(sendBuff), 0);
    if (SOCKET_ERROR == bytesSent)
    {
        cout << "Web Server: Error at send(): " << WSAGetLastError() << endl;
        return;
    }

    cout << "Web Server: Sent: " << bytesSent << "\\" << strlen(sendBuff) << " bytes of \"" << sendBuff << "\" message.\n";

    sockets[index].send = IDLE;

    // Reset lastIndex after sending the response
    sockets[index].lastIndex = 0;
}

void makeHeader(string& response, string status, string contentType)
{
    response = "HTTP/1.1 " + status + lineSuffix;
    response += "Server: HTTP Web Server" + lineSuffix;
    response += "Content-Type: " + contentType + lineSuffix;
}

void makeBody(string& response, string body)
{
    response += "Content-Length: " + to_string(body.length()) + lineSuffix + lineSuffix;
    response += body;
}

void GetMethodHandler(string& response, string& socketBuffer, const char* queryString = nullptr)
{
    string status = "200 OK";
    string resourcePath = "/index.html";
    string contentType = "text/html";
    string HTMLContent;

    if (queryString != nullptr)
    {
        const char* langParam = strstr(queryString, "lang=");
        if (langParam != nullptr)
        {
            langParam += 5;
            if (strncmp(langParam, "he", 2) == 0)
                HTMLContent = "<html><body><h1>!שלום עולם</h1></body></html>";
            else if (strncmp(langParam, "fr", 2) == 0)
                HTMLContent = "<html><body><h1>Bonjour le monde!</h1></body></html>";
            else
                HTMLContent = "<html><body><h1>Hello World!</h1></body></html>";
        }
    }

    makeHeader(response, status, contentType);
    makeBody(response, HTMLContent);
}

void HeadMethodHandler(string& response, string& socketBuffer)
{
    GetMethodHandler(response, socketBuffer);
}

void OptionsMethodHandler(string& response)
{
    string status = "200 OK";
    string contentType = "text/html";
    string body = "Supported methods: OPTIONS, GET, HEAD, POST, PUT, DELETE, TRACE";

    response = "HTTP/1.1 " + status + lineSuffix;
    response += "Allow: OPTIONS, GET, HEAD, POST, PUT, DELETE, TRACE" + lineSuffix;
    response += "Content-Type: " + contentType + lineSuffix;
    response += "Content-Length: " + to_string(body.length()) + lineSuffix + lineSuffix;
    response += body;
}

void NotAllowMethodHandler(string& response)
{
    string status = "405 Method Not Allowed";
    string contentType = "text/html";

    makeHeader(response, status, contentType);
    response += "Allow: OPTIONS, GET, HEAD, POST, PUT, DELETE, TRACE" + lineSuffix;
    response += "Connection: close" + lineSuffix + lineSuffix;
}

void TraceMethodHandler(string& response, string& socketBuffer)
{
    string status = "200 OK";
    string contentType = "message/http";
    string traceString = "TRACE " + socketBuffer;

    makeHeader(response, status, contentType);
    makeBody(response, traceString);
}

void PutMethodHandler(string& response, const char* body)
{
    string status = "200 OK";
    string contentType = "text/html";
    string putBody = makePostBody(body);

    makeHeader(response, status, contentType);
    makeBody(response, putBody);
}

string escapeHTML(const string& input)
{
    string output;
    for (char c : input)
    {
        switch (c)
        {
        case '&': output += "&amp;"; break;
        case '<': output += "&lt;"; break;
        case '>': output += "&gt;"; break;
        case '"': output += "&quot;"; break;
        case '\'': output += "&apos;"; break;
        default: output += c; break;
        }
    }
    return output;
}

string makePostBody(const string& body)
{
    ostringstream htmlBody;

    string escapedBody = escapeHTML(body);

    htmlBody << "<!DOCTYPE html>\n";
    htmlBody << "<html>\n" << "<head>\n";
    htmlBody << "<title>Post Method</title>\n";
    htmlBody << "</head>\n" << "<body>\n";
    htmlBody << "<h1>POST Succeeded</h1>\n";
    htmlBody << "<p>The Content is \"" << escapedBody << "\".</p>\n";
    htmlBody << "</body>\n" << "</html>\n";

    return htmlBody.str();
}

void PostMethodHandler(string& response, const char* body)
{
    string status = "200 OK";
    string contentType = "text/html";
    string postBody = makePostBody(body);

    makeHeader(response, status, contentType);
    makeBody(response, postBody);
}

void DeleteMethodHandler(string& response, string& socketBuffer)
{
    string status = "200 OK";
    string contentType = "text/html";
    string body = "<html><body><h1>Resource Deleted Successfully</h1></body></html>";

    makeHeader(response, status, contentType);
    makeBody(response, body);
}

void getCommand(char* sendBuff, int& bytesSent, SocketState curSocket)
{
    string response;
    string socketBuffer = "GET /index.html HTTP/1.1\r\nHost: localhost\r\nAccept: text/html\r\n\r\n";
    GetMethodHandler(response, socketBuffer, curSocket.buffer);
    bytesSent = response.size();
    strcpy(sendBuff, response.c_str());
}

void headCommand(char* sendBuff, int& bytesSent)
{
    string response;
    string socketBuffer = "HEAD /index.html HTTP/1.1\r\nHost: localhost\r\nAccept: text/html\r\n\r\n";
    HeadMethodHandler(response, socketBuffer);
    bytesSent = response.size();
    strcpy(sendBuff, response.c_str());
}

void putCommand(char* sendBuff, int& bytesSent, const char* body)
{
    string response;
    PutMethodHandler(response, body);
    bytesSent = response.size();
    strcpy(sendBuff, response.c_str());
}

void optionsCommand(char* sendBuff, int& bytesSent)
{
    string response;
    OptionsMethodHandler(response);
    bytesSent = response.size();
    strcpy(sendBuff, response.c_str());
}

void postCommand(char* sendBuff, int& bytesSent, const char* body)
{
    string response;
    PostMethodHandler(response, body);
    bytesSent = response.size();
    strcpy(sendBuff, response.c_str());
}

void deleteCommand(char* sendBuff, int& bytesSent)
{
    string response;
    string socketBuffer = "DELETE /resource HTTP/1.1\r\nHost: localhost\r\nAccept: text/html\r\n\r\n";
    DeleteMethodHandler(response, socketBuffer);
    bytesSent = response.size();
    strcpy(sendBuff, response.c_str());
}

void traceCommand(char* sendBuff, int& bytesSent, SocketState socket)
{
    string response;
    string socketBuffer = "TRACE /index.html HTTP/1.1\r\nHost: localhost\r\nAccept: text/html\r\n\r\n";
    TraceMethodHandler(response, socketBuffer);
    bytesSent = response.size();
    strcpy(sendBuff, response.c_str());
}
*/