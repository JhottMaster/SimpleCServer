#include "stdafx.h"
#include "Server.h"
#include <conio.h>

int main()
{
	int serverPort = 4678;
	{
		Server* serverHandle = new Server(serverPort);
		printf("Server setup and listening on port %d. Press any key to quit.\n", serverPort);

		while (!_kbhit())
		{
			unsigned int clientID = serverHandle->acceptNewClients();
			if (clientID > 0) printf("\nNew connection from IP %s. (ID %d)\n", serverHandle->sessions[clientID]->IPAddress, clientID);

			// Process our existing clients
			for (auto it = serverHandle->sessions.cbegin(); it != serverHandle->sessions.cend();)
			{
				Connection* con = serverHandle->sessions[it->first];

				bool deleteConnection = false;
				if (con->ReadNetworkMessageSize() > 0)
				{
					if (con->NetworkMessageIsComplete('\r'))
					{
						char* message = con->GetNetworkMessage();
						printf("\nNew message from client %d: '%s'", con->connectionID, message);
						if (strcmp(message, "exit") == 0)
						{
							printf("\nClient %d: requested disconnect.", con->connectionID);
							con->Shutdown();
							deleteConnection = true;
						}

						int msgLength = -1; while (message[++msgLength] != '\0' && msgLength < NETWORK_BUFFER_SIZE);
						char* relayMessage = new char[NETWORK_BUFFER_SIZE];
						if (strstr(message, "echo") != nullptr)
						{
							strncpy_s(relayMessage, msgLength - 3, message + 4, msgLength - 4);
							con->SendNetworkMessage(message, msgLength);
						}
						else
						{
							int relayMessageSize = sprintf_s(relayMessage, NETWORK_BUFFER_SIZE, "User %d (%s): %s\n\r\n\r", con->connectionID, con->IPAddress, message);
							for (auto subIt = serverHandle->sessions.cbegin(); subIt != serverHandle->sessions.cend();++subIt)
								serverHandle->sessions[subIt->first]->SendNetworkMessage(relayMessage, relayMessageSize);
						}
					}
				}

				if (deleteConnection)
				{
					serverHandle->sessions.erase(it++);
				}
				else
				{
					++it;
				}
			}
		}
	}
}

