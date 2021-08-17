#include "HttpWebServer.h"

HttpWebServer::HttpWebServer(const char* ip_address, int port) {
	tcp_ip_address = ip_address;
	tcp_port = port;
}

// Output Winsock error message from error code
void HttpWebServer::outputErrorMessage(int error_code) {
	std::cout << "test";
	wchar_t buffer[256];
	buffer[0] = '\0';
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		error_code,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		buffer,
		sizeof(buffer),
		NULL);
	std::wcout << buffer << ", " << error_code;
}

std::string HttpWebServer::httpRequestHeader(int status_code, std::string status_message,  int size) {
	std::string htmlRequest = "HTTP/1.1 " + std::to_string(status_code) + " " + status_message + "\r\nCache-Control: no-cache, private\r\nContent-Type: text/html\r\nContent-Length: " + std::to_string(size) + "\r\n\r\n";
	std::cout << htmlRequest;
	return htmlRequest;
}

// Initialize use of Winsock DLL
void HttpWebServer::initializeWinsock() {
	WSADATA ws_data;
	WORD ver = MAKEWORD(2, 2);

	int ws_init = WSAStartup(ver, &ws_data);
	if (ws_init != 0) {
		throw ws_init;
	}
}

// Create a socket with an address family of IP and a socket type
// of virtual circuit service
void HttpWebServer::createAndAssignSocket() {
	tcp_socket = socket(AF_INET, SOCK_STREAM, 0);

	if (tcp_socket == INVALID_SOCKET) {
		throw WSAGetLastError();
	}
}

// Bind ip address and port to the socket
void HttpWebServer::bindToSocket() {
	sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_port = htons(tcp_port);
	inet_pton(AF_INET, tcp_ip_address, &address.sin_addr);

	if (bind(tcp_socket, (sockaddr*)&address, sizeof(address)) == -1) {
		throw WSAGetLastError();
	}
}

// Begin listening for incoming connection on socket
void HttpWebServer::listenOnSocket() {
	if (listen(tcp_socket, SOMAXCONN) == -1) {
		throw WSAGetLastError();
	}
}


// Add existing socket to the master file descriptor to hear incoming connections
void HttpWebServer::addSocketToFileDescriptor() {
	FD_ZERO(&tcp_master);
	FD_SET(tcp_socket, &tcp_master);
}

void HttpWebServer::initAndRun() {
	// Initialization of TCP listener
	try {
		initializeWinsock();
		createAndAssignSocket();
		bindToSocket();
		listenOnSocket();
		addSocketToFileDescriptor();
	}
	catch (int e) {
		throw e;
	}
	std::cout << "HTTP Web Server initialized.\n";
	std::cout << "Running on port " << tcp_port << "...\n\n";

	// Run TCP listener
	while (1) {
		fd_set copy_fd = tcp_master;
		int socket_count = select(0, &copy_fd, NULL, NULL, NULL);

		for (int i = 0; i < socket_count; i++)
		{
			SOCKET current_socket = copy_fd.fd_array[i];

			// Accept a new inbound connection
			if (current_socket == tcp_socket) {
				SOCKET client = accept(tcp_socket, NULL, NULL);
				FD_SET(client, &tcp_master);
			}
			// Accept a new inbound message
			else {
				const int BUFFER_SIZE = 4096;
				char buffer[BUFFER_SIZE];
				int bytes = recv(current_socket, buffer, BUFFER_SIZE, 0);

				// Close the connection if no data is in the message
				if (bytes <= 0) {
					closesocket(current_socket);
					FD_CLR(current_socket, &tcp_master);
				}
				// Send inbound message to all other clients
				else {
					// Parse html out of file
					std::istringstream istream(buffer);
					std::vector<std::string> parsed_string((std::istream_iterator<std::string>(istream)), std::istream_iterator<std::string>());

					// Get the requested html file from the file system
					int status_code = 404;
					std::string status_message = "Not Found";
					std::string html_content = "<h1 style='color: red; font-family: sans-serif;'>Error 404: Not Found</h1>";

					if (parsed_string.size() >= 3 && parsed_string[0] == "GET") {
						std::string path = (parsed_string[1] == "/") ? "/index.html" : parsed_string[1];
						std::ifstream file(".\\wwwroot" + path);

						if (file.good()) {
							std::string strContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
							html_content = strContent;
							status_code = 200;
							status_message = "OK";
						}
						file.close();
					}

					// Write Http Request Header and HTML content to the stream and send to client

					std::ostringstream stream;

					stream << httpRequestHeader(status_code, status_message, html_content.size())
						<< html_content;

					send(current_socket, stream.str().c_str(), stream.str().size() + 1, 0);
				}
			}
		}
	}

	// Close the listening socket so that no one else can connect
	FD_CLR(tcp_socket, &tcp_master);
	closesocket(tcp_socket);

	// End all socket connections
	while (tcp_master.fd_count > 0) {
		SOCKET current_socket = tcp_master.fd_array[0];

		FD_CLR(current_socket, &tcp_master);
		closesocket(current_socket);
	}

	WSACleanup();
}