#define _CRT_SECURE_NO_WARNINGS
#include "http.h"
#include <sstream>
#include <iostream>

const string lineSuffix = "\r\n";

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

void GetMethodHandler(string& response, string& socketBuffer, const char* queryString)
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

void optionsCommand(char* sendBuff, int& bytesSent, const char* buffer)
{
    string response = "HTTP/1.1 200 OK\r\n";
    response += "Allow: OPTIONS, GET, HEAD, POST, PUT, DELETE, TRACE\r\n";
    response += "Content-Type: text/html\r\n";
    response += "Content-Length: 0\r\n\r\n";

    bytesSent = response.size();
    strcpy(sendBuff, response.c_str());
}


void getCommand(char* sendBuff, int& bytesSent, const char* buffer)
{
    string response;
    char method[8], query[128], body[256];

    // Parse the buffer in the format <method>\r\n<query>\r\n<body>\0
    sscanf(buffer, "%s\r\n%s\r\n%s", method, query, body);

    // If no query parameter present, default to English
    if (strcmp(query, "NO_QUERY") == 0)
    {
        strcpy(query, "lang=en");
    }

    // Extract the value of the "lang" parameter
    char lang[32] = "en";
    char* langParam = strstr(query, "lang=");
    if (langParam != nullptr)
    {
        strcpy(lang, langParam + 5);
    }

    // Determine the response based on the lang parameter
    if (strcmp(lang, "he") == 0)
    {
        response = "<html><body><h1>!שלום עולם</h1></body></html>";
    }
    else if (strcmp(lang, "fr") == 0)
    {
        response = "<html><body><h1>Bonjour le monde!</h1></body></html>";
    }
    else // Default to English
    {
        response = "<html><body><h1>Hello World!</h1></body></html>";
    }

    // Construct the HTTP response
    string httpResponse = "HTTP/1.1 200 OK\r\n";
    httpResponse += "Content-Type: text/html\r\n";
    httpResponse += "Content-Length: " + to_string(response.length()) + "\r\n\r\n";
    httpResponse += response;

    bytesSent = httpResponse.size();
    strcpy(sendBuff, httpResponse.c_str());
}




void headCommand(char* sendBuff, int& bytesSent, const char* buffer)
{
    string response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: text/html\r\n";
    response += "Content-Length: 0\r\n\r\n";

    bytesSent = response.size();
    strcpy(sendBuff, response.c_str());
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


void postCommand(char* sendBuff, int& bytesSent, const char* buffer)
{
    char method[8], query[128], body[512];
    char* bufferCopy = _strdup(buffer);
    // Extract the method
    char* methodEnd = strchr(bufferCopy, '\r');
    if (methodEnd != nullptr)
    {
        *methodEnd = '\0';
        strcpy(method, bufferCopy);
    }

    // Extract the query
    char* queryStart = methodEnd + 2; // Skip "\r\n"
    char* queryEnd = strchr(queryStart, '\r');
    if (queryEnd != nullptr)
    {
        *queryEnd = '\0';
        strcpy(query, queryStart);
    }

    // Extract the body
    char* bodyStart = queryEnd + 2; // Skip "\r\n"
    strcpy(body, bodyStart);

    std::cout << "Web Server: POST Body: " << body << std::endl;


    string status = "200 OK";
    string contentType = "text/html";
    string postBody = makePostBody(body);

    makeHeader(response, status, contentType);
    makeBody(response, postBody);





    string response = "HTTP/1.1 201 Created\r\n";
    response += "Content-Type: text/html\r\n";
    response += "Content-Length: " + to_string(strlen(body)) + "\r\n\r\n";
    response += makePostBody(string(body));

    bytesSent = response.size();
    strcpy(sendBuff, response.c_str());
    free(bufferCopy);
}





void putCommand(char* sendBuff, int& bytesSent, const char* buffer)
{
    char method[8], query[128], body[256];

    // Parse the buffer in the format <method>\r\n<query>\r\n<body>\0
    sscanf(buffer, "%s\r\n%s\r\n%[^\r\n]", method, query, body);

    std::cout << "Web Server: PUT Body: " << body << std::endl;

    string response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: text/html\r\n";
    response += "Content-Length: " + to_string(strlen(body)) + "\r\n\r\n";
    response += "<html><body><h1>PUT Succeeded</h1><p>The Content is \"" + string(body) + "\"</p></body></html>";

    bytesSent = response.size();
    strcpy(sendBuff, response.c_str());
}




void deleteCommand(char* sendBuff, int& bytesSent, const char* buffer)
{
    string response = "<html><body><h1>Resource Deleted Successfully</h1></body></html>";

    string httpResponse = "HTTP/1.1 200 OK\r\n";
    httpResponse += "Content-Type: text/html\r\n";
    httpResponse += "Content-Length: " + to_string(response.length()) + "\r\n\r\n";
    httpResponse += response;

    bytesSent = httpResponse.size();
    strcpy(sendBuff, httpResponse.c_str());
}



void traceCommand(char* sendBuff, int& bytesSent, const char* buffer)
{
    char method[8], query[128], body[256];

    // Parse the buffer in the format <method>\r\n<query>\r\n<body>\0
    sscanf(buffer, "%s\r\n%s\r\n%[^\r\n]", method, query, body);

    string traceContent = "TRACE " + string(buffer);
    string response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: message/http\r\n";
    response += "Content-Length: " + to_string(traceContent.length()) + "\r\n\r\n";
    response += traceContent;

    bytesSent = response.size();
    strcpy(sendBuff, response.c_str());
}
