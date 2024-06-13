#include "httpCommands.h"


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
//void OptionsMethodHandler(string& response) {
//	string status = "204 no content";
//	string contentType = "text/html";
//
//	makeHeader(response, status, contentType);
//	response += "Supported methods: OPTIONS, GET, HEAD, POST, PUT, DELETE, TRACE" + lineSuffix;
//}
void OptionsMethodHandler(string& response)
{
	string status = "200 OK";
	string contentType = "text/html";
	string body = "Supported methods: OPTIONS, GET, HEAD, POST, PUT, DELETE, TRACE";

	// Create the response header
	response = "HTTP/1.1 " + status + lineSuffix;
	response += "Allow: OPTIONS, GET, HEAD, POST, PUT, DELETE, TRACE" + lineSuffix;
	response += "Content-Type: " + contentType + lineSuffix;
	response += "Content-Length: " + to_string(body.length()) + lineSuffix + lineSuffix;
	response += body;
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

void getCommand(char* sendBuff, int& bytesSent, SocketState curSocket) {
	string response;
	string socketBuffer = "GET /index.html HTTP/1.1\r\nHost: localhost\r\nAccept: text/html\r\n\r\n";
	GetMethodHandler(response, socketBuffer, curSocket.buffer);
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




