#include "HttpWebServer.h"

void main() {
	HttpWebServer webServer("0.0.0.0", 8080);
	try {
		webServer.initAndRun();
	}
	catch (int e) {
		webServer.outputErrorMessage(0);
	}

	std::cin.get();
}