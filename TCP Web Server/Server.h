#ifndef SERVER_H
#define SERVER_H
#ifdef DELETE
#undef DELETE
#endif

#include <winsock2.h>

enum HTTPRequestTypes {
    OPTIONS_R = 1,
    GET_R = 2,
    HEAD_R = 3,
    POST_R = 4,
    PUT_R = 5,
    DELETE_R = 6,
    TRACE_R = 7
};

struct SocketState
{
    SOCKET id;            // Socket handle
    int recv;            // Receiving?
    int send;            // Sending?
    HTTPRequestTypes sendSubType;    // Sending sub-type
    char buffer[1024];
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

bool addSocket(SOCKET id, int what);
void removeSocket(int index);
void acceptConnection(int index);
void receiveMessage(int index);
void sendMessage(int index);

extern SocketState sockets[MAX_SOCKETS];
extern int socketsCount;

#endif // SERVER_H
