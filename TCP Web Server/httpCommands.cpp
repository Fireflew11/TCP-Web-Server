#include "httpCommands.h"
#include <direct.h>  // For _mkdir
#include <fstream>   // For file stream operations
#include <ctime>     // For time operations

static std::string baseDirectory;

void initializeBaseDirectory() {
    wchar_t wCurrentPath[MAX_PATH];
    if (GetCurrentDirectoryW(MAX_PATH, wCurrentPath)) {
        // Convert wide string to a regular string
        char currentPath[MAX_PATH];
        wcstombs(currentPath, wCurrentPath, MAX_PATH);
        baseDirectory = std::string(currentPath);
    }
}



void makeHeader(string& response, string status, string contentType) {
    response = "HTTP/1.1 " + status + lineSuffix;
    response += "Server: HTTP Web Server" + lineSuffix;
    response += "Content-Type: " + contentType + lineSuffix;
}

void makeBody(string& response, string body) {
    response += "Content-Length: " + to_string(body.length()) + lineSuffix + lineSuffix;
    response += body;
}

void GetMethodHandler(string& response, const SocketState& state) {
    string status = "200 OK";
    string resourcePath = "/index.html";
    string contentType = "text/html";
    string HTMLContent;

    string lang = getLanguageFromQuery(state.buffer);
    if (lang == "he")
        HTMLContent = "<html><body><h1>!שלום עולם</h1></body></html>";
    else if (lang == "fr")
        HTMLContent = "<html><body><h1>Bonjour le monde!</h1></body></html>";
    else
        HTMLContent = "<html><body><h1>Hello World!</h1></body></html>";

    makeHeader(response, status, contentType);
    makeBody(response, HTMLContent);
}

void HeadMethodHandler(string& response, const SocketState& state) {
    GetMethodHandler(response, state);
    response = response.substr(0, response.find("\r\n\r\n") + 4);
}

void OptionsMethodHandler(string& response, const SocketState& state) {
    string status = "200 OK";
    string contentType = "text/html";
    string body = "Supported methods: OPTIONS, GET, HEAD, POST, PUT, DELETE, TRACE";

    response = "HTTP/1.1 " + status + lineSuffix;
    response += "Allow: OPTIONS, GET, HEAD, POST, PUT, DELETE, TRACE" + lineSuffix;
    response += "Content-Type: " + contentType + lineSuffix;
    response += "Content-Length: " + to_string(body.length()) + lineSuffix + lineSuffix;
    response += body;
}

void NotAllowMethodHandler(string& response, const SocketState& state) {
    string status = "405 Method Not Allowed";
    string contentType = "text/html";

    makeHeader(response, status, contentType);
    response += "Allow: OPTIONS, GET, HEAD, POST, PUT, DELETE, TRACE" + lineSuffix;
    response += "Connection: close" + lineSuffix + lineSuffix;
}

void TraceMethodHandler(string& response, const SocketState& state) {
    string status = "200 OK";
    string contentType = "message/http";
    string traceString = "TRACE " + state.buffer;

    makeHeader(response, status, contentType);
    makeBody(response, traceString);
}

void PutMethodHandler(string& response, const SocketState& state) {
    string status = "201 Created";
    string contentType = "text/html";
    string body = state.buffer;

    makeHeader(response, status, contentType);
    makeBody(response, body);
}

string makePostBody(const string& body) {
    ostringstream htmlBody;
    htmlBody << "<!DOCTYPE html>\n";
    htmlBody << "<html>\n<head>\n";
    htmlBody << "<title>Post Method</title>\n";
    htmlBody << "</head>\n<body>\n";
    htmlBody << "<h1>POST Succeeded</h1>\n";
    htmlBody << "<p>The Content is \"<a>" << body << " \"</a>.</p>\n";
    htmlBody << "</body>\n</html>\n";

    return htmlBody.str();
}

void PostMethodHandler(string& response, const SocketState& state) {
    std::string filename = std::to_string(std::time(nullptr)) + ".txt";
    std::string dirPath = baseDirectory + "\\" + RESOURCE_PATH;
    std::string filePath = dirPath + "\\" + filename;

    string status = "201 Created";
    string contentType = "text/html";
    string postBody = makePostBody(filename);

    // Generate a unique filename based on the current time
    
    // Ensure the Resources directory exists
    if (_mkdir(dirPath.c_str()) != 0 && errno != EEXIST) {
        status = "500 Internal Server Error";
        postBody = "<html><body><h1>Failed to create directory</h1></body></html>";
    }
    else {
        // Write the body content to a new file
        FILE* outFile = fopen(filePath.c_str(), "w");
        if (outFile != nullptr) {
            fwrite(state.body.c_str(), sizeof(char), state.body.size(), outFile);
            fclose(outFile);
            std::cout << "File created successfully: " << filePath << std::endl; // Debug output
        }
        else {
            status = "500 Internal Server Error";
            postBody = "<html><body><h1>Failed to create file</h1></body></html>";
            std::cout << "Failed to open file: " << filePath << std::endl; // Debug output
        }
    }

    makeHeader(response, status, contentType);
    makeBody(response, postBody);
}



void DeleteMethodHandler(string& response, const SocketState& state) {
    string status = "200 OK";
    string contentType = "text/html";
    string body = "<html><body><h1>Resource Deleted Successfully</h1></body></html>";

    makeHeader(response, status, contentType);
    makeBody(response, body);
}

void getCommand(char* sendBuff, int& bytesSent, const SocketState& state) {
    string response;
    GetMethodHandler(response, state);
    bytesSent = response.size();
    strcpy(sendBuff, response.c_str());
}

void headCommand(char* sendBuff, int& bytesSent, const SocketState& state) {
    string response;
    HeadMethodHandler(response, state);
    bytesSent = response.size();
    strcpy(sendBuff, response.c_str());
}

void postCommand(char* sendBuff, int& bytesSent, const SocketState& state) {
    string response;
    PostMethodHandler(response, state);
    bytesSent = response.size();
    strcpy(sendBuff, response.c_str());
}

void optionsCommand(char* sendBuff, int& bytesSent, const SocketState& state) {
    string response;
    OptionsMethodHandler(response, state);
    bytesSent = response.size();
    strcpy(sendBuff, response.c_str());
}

void putCommand(char* sendBuff, int& bytesSent, const SocketState& state) {
    string response;
    PutMethodHandler(response, state);
    bytesSent = response.size();
    strcpy(sendBuff, response.c_str());
}

void deleteCommand(char* sendBuff, int& bytesSent, const SocketState& state) {
    string response;
    DeleteMethodHandler(response, state);
    bytesSent = response.size();
    strcpy(sendBuff, response.c_str());
}

void traceCommand(char* sendBuff, int& bytesSent, const SocketState& state) {
    string response;
    TraceMethodHandler(response, state);
    bytesSent = response.size();
    strcpy(sendBuff, response.c_str());
}

const string getLanguageFromQuery(const string& buffer) {
    size_t queryPos = buffer.find('?');
    string lang = "en";  // default language

    if (queryPos != string::npos) {
        size_t langPos = buffer.find("lang=", queryPos);
        if (langPos != string::npos) {
            size_t start = langPos + 5; // length of "lang=" is 5
            size_t end = buffer.find('&', start);
            if (end == string::npos) {
                end = buffer.length();
            }
            lang = buffer.substr(start, end - start);

            if (lang != "en" && lang != "fr" && lang != "he") {
                lang = "en";  // fallback to default language
            }
        }
    }

    return lang;
}

const string getRequestBody(const string& buffer) {
    size_t bodyPos = buffer.find(lineSuffix + lineSuffix);
    if (bodyPos != string::npos) {
        return buffer.substr(bodyPos + 4); // +4 to skip past the "\r\n\r\n"
    }
    return "";
}
