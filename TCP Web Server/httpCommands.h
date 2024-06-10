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


const int SOCKET_BUFFER_SIZE = 2048;
const std::string ROOT_DIRECTORY = "C://temp/"; // default working directory
const std::string DEFAULT_RESOURCE = "/index.html"; // default index

enum LANG {
	en,
	he,
	fr
};

