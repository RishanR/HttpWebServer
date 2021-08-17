#pragma once
#pragma comment(lib, "Ws2_32.lib")

#include <iostream>
#include <string>
#include <sstream>
#include <WS2tcpip.h>
#include <wchar.h>
#include <fstream>
#include <vector>

class HttpWebServer {
public:
	// Constructor to initialize ip address and port of listener
	HttpWebServer(const char* ip_address, int port);

	// Initialization and run the listener
	void initAndRun();
	void outputErrorMessage(int error_code);
private:
	const char* tcp_ip_address;
	int tcp_port;
	int tcp_socket;
	fd_set tcp_master;
	void initializeWinsock();
	void createAndAssignSocket();
	void bindToSocket();
	void listenOnSocket();
	void addSocketToFileDescriptor();
	std::string httpRequestHeader(int status_code, std::string status_message, int size);
};
