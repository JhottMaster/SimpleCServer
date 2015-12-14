#include "stdafx.h"
#include "Server.h"

Server::Server(int defaultPort = 8439)
{
	// create WSADATA object
	WSADATA wsaData;

	// our sockets for the server
	ListenSocket = INVALID_SOCKET;

	// address info for the server to listen to
	struct addrinfo *result = NULL;
	struct addrinfo hints;

	// Initialize Winsock
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
		exit(1);
	}

	// set address information
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;    // TCP connection!!!
	hints.ai_flags = AI_PASSIVE;

	// Resolve the server address and port
	char tmpBuffer[16]; 
	_itoa_s(defaultPort, tmpBuffer, 16, 10);
	PCSTR portNumAsStr = tmpBuffer;
	iResult = getaddrinfo(NULL, portNumAsStr, &hints, &result);

	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		exit(1);
	}

	// Create a SOCKET for connecting to server
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

	if (ListenSocket == INVALID_SOCKET) {
		printf("socket failed with error: %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		exit(1);
	}

	// Set the mode of the socket to be nonblocking
	u_long iMode = 1;
	iResult = ioctlsocket(ListenSocket, FIONBIO, &iMode);

	if (iResult == SOCKET_ERROR) {
		printf("ioctlsocket failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		exit(1);
	}

	// Setup the TCP listening socket
	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);

	if (iResult == SOCKET_ERROR) {
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		exit(1);
	}

	// no longer need address information
	freeaddrinfo(result);

	// start listening for new clients attempting to connect
	iResult = listen(ListenSocket, SOMAXCONN);

	if (iResult == SOCKET_ERROR) {
		printf("listen failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		exit(1);
	}
}

unsigned int Server::acceptNewClients()
{
	// Create the socket type we need to get client information such as IP address:
	struct sockaddr_in client_info = { 0 };
	int sizeOfSock = sizeof(client_info);

	// Listen for and accept pending client connections:
	SOCKET ClientSocket = accept(ListenSocket, (sockaddr*)&client_info /* this method takes sockaddr instead of sockaddr_in structure, so we cast it */ , &sizeOfSock);

	if (ClientSocket != INVALID_SOCKET)
	{
		//Disable nagle algorithim on the socket. (Nagle algorithm reduces netowrk traffict by buffering small packets to combine them and then flush them all at once)
		char value = 1;
		setsockopt(ClientSocket, IPPROTO_TCP, TCP_NODELAY, &value, sizeof(value));

		// Create a new Connection object to represent this connection/session and add it to our table of connections:
		Connection* ptr = new Connection(IDCounter, ClientSocket, client_info);
		sessions.insert(std::pair<unsigned int, Connection*>(IDCounter, ptr));

		return IDCounter++;
	}
	return 0;
}

// When destroying this class, close all connections:
Server::~Server()
{
	// Close out any existing connections...
	printf("\nClosing connections...");
	for (std::map<unsigned int, Connection*>::iterator iter = sessions.begin(); iter != sessions.end(); ++iter)
		sessions[iter->first]->Shutdown();
}


