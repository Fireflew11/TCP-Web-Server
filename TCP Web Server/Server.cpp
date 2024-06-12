#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <iostream>
#include <winsock2.h>
#include "server.h"
#include "http.h"

#pragma comment(lib, "Ws2_32.lib")

using namespace std;

SocketState sockets[MAX_SOCKETS] = { 0 };
int socketsCount = 0;

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

        std::cout << "Web Server: Received: " << bytesRecv << " bytes of \"" << sockets[index].buffer << "\" message.\n";

        // Extract the request line
        char* requestLineEnd = strstr(sockets[index].buffer, "\r\n");
        if (requestLineEnd != nullptr)
        {
            *requestLineEnd = '\0';
            char* requestLine = sockets[index].buffer;

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
                sockets[index].sendSubType = HTTPRequestTypes::OPTIONS_R;
                sprintf(sockets[index].buffer, "%s\r\nNO_QUERY\r\nNO_BODY", method);
            }
            else if (strcmp(method, "GET") == 0)
            {
                sockets[index].send = SEND;
                sockets[index].sendSubType = HTTPRequestTypes::GET_R;

                // Extract the query string from the URL
                char* queryString = strchr(url, '?');
                char query[128] = "NO_QUERY";
                if (queryString != nullptr)
                {
                    queryString++; // Move to the query string part
                    strcpy(query, queryString);
                }
                sprintf(sockets[index].buffer, "%s\r\n%s\r\nNO_BODY", method, query);
            }
            else if (strcmp(method, "HEAD") == 0)
            {
                sockets[index].send = SEND;
                sockets[index].sendSubType = HTTPRequestTypes::HEAD_R;
                sprintf(sockets[index].buffer, "%s\r\nNO_QUERY\r\nNO_BODY", method);
            }
            else if (strcmp(method, "POST") == 0)
            {
                sockets[index].send = SEND;
                sockets[index].sendSubType = HTTPRequestTypes::POST_R;

                char* body = strstr(requestLineEnd + 2, "\r\n\r\n");
                char bodyContent[256] = "NO_BODY";
                if (body != nullptr)
                {
                    body += 4; // Skip the "\r\n\r\n"
                    strcpy(bodyContent, body);
                }
                sprintf(sockets[index].buffer, "%s\r\nNO_QUERY\r\n%s", method, bodyContent);
            }
            else if (strcmp(method, "PUT") == 0)
            {
                sockets[index].send = SEND;
                sockets[index].sendSubType = HTTPRequestTypes::PUT_R;

                char* body = strstr(requestLineEnd + 2, "\r\n\r\n");
                char bodyContent[256] = "NO_BODY";
                if (body != nullptr)
                {
                    body += 4; // Skip the "\r\n\r\n"
                    strcpy(bodyContent, body);
                }
                sprintf(sockets[index].buffer, "%s\r\nNO_QUERY\r\n%s", method, bodyContent);
            }
            else if (strcmp(method, "DELETE") == 0)
            {
                sockets[index].send = SEND;
                sockets[index].sendSubType = HTTPRequestTypes::DELETE_R;
                sprintf(sockets[index].buffer, "%s\r\nNO_QUERY\r\nNO_BODY", method);
            }
            else if (strcmp(method, "TRACE") == 0)
            {
                sockets[index].send = SEND;
                sockets[index].sendSubType = HTTPRequestTypes::TRACE_R;
                sprintf(sockets[index].buffer, "%s\r\nNO_QUERY\r\nNO_BODY", method);
            }
            else if (strcmp(method, "Exit") == 0)
            {
                closesocket(msgSocket);
                removeSocket(index);
                return;
            }

            sockets[index].len = strlen(sockets[index].buffer);
        }
    }
}


void sendMessage(int index)
{
    int bytesSent = 0;
    char sendBuff[512];

    SOCKET msgSocket = sockets[index].id;
    switch (sockets[index].sendSubType)
    {
    case HTTPRequestTypes::OPTIONS_R:
        optionsCommand(sendBuff, bytesSent, sockets[index].buffer);
        break;

    case HTTPRequestTypes::GET_R:
        getCommand(sendBuff, bytesSent, sockets[index].buffer);
        break;

    case HTTPRequestTypes::HEAD_R:
        headCommand(sendBuff, bytesSent, sockets[index].buffer);
        break;

    case HTTPRequestTypes::POST_R:
        postCommand(sendBuff, bytesSent, sockets[index].buffer);
        break;

    case HTTPRequestTypes::PUT_R:
        putCommand(sendBuff, bytesSent, sockets[index].buffer);
        break;

    case HTTPRequestTypes::DELETE_R:
        deleteCommand(sendBuff, bytesSent, sockets[index].buffer);
        break;

    case HTTPRequestTypes::TRACE_R:
        traceCommand(sendBuff, bytesSent, sockets[index].buffer);
        break;
    }

    bytesSent = send(msgSocket, sendBuff, (int)strlen(sendBuff), 0);
    if (SOCKET_ERROR == bytesSent)
    {
        cout << "Web Server: Error at send(): " << WSAGetLastError() << endl;
        return;
    }

    cout << "Web Server: Sent: " << bytesSent << "\\" << strlen(sendBuff) << " bytes of \"" << sendBuff << "\" message.\n";

    sockets[index].send = IDLE;

    // Move remaining data in the buffer to the beginning
    int handledMessageLength = sockets[index].len;
    int remainingDataLength = sockets[index].lastIndex - handledMessageLength;
    if (remainingDataLength > 0)
    {
        memmove(sockets[index].buffer, sockets[index].buffer + handledMessageLength, remainingDataLength);
    }
    sockets[index].lastIndex = remainingDataLength;
}



int main()
{
    WSAData wsaData;
    if (NO_ERROR != WSAStartup(MAKEWORD(2, 2), &wsaData))
    {
        cout << "Web Server: Error at WSAStartup()\n";
        return 1;
    }

    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (INVALID_SOCKET == listenSocket)
    {
        cout << "Web Server: Error at socket(): " << WSAGetLastError() << endl;
        WSACleanup();
        return 1;
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
        return 1;
    }

    if (SOCKET_ERROR == listen(listenSocket, 5))
    {
        cout << "Time Server: Error at listen(): " << WSAGetLastError() << endl;
        closesocket(listenSocket);
        WSACleanup();
        return 1;
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

        int nfd = select(0, &waitRecv, &waitSend, NULL, NULL);
        if (nfd == SOCKET_ERROR)
        {
            cout << "Web Server: Error at select(): " << WSAGetLastError() << endl;
            WSACleanup();
            return 1;
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

    cout << "Web Server: Closing Connection.\n";
    closesocket(listenSocket);
    WSACleanup();
    return 0;
}
