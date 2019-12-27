#include <WS2tcpip.h>
#include <iostream>
#include <spdlog/spdlog.h>
#include <thread>
#include <vector>
#include <string>
#include <mutex>

#pragma comment(lib, "ws2_32.lib")

bool TestMode = false;
std::mutex ClientMTX;
unsigned int firstTestClient;
unsigned int secondTestClient;

struct SocketStruct
{
	sockaddr_in socketObj;
	int socketSize;
	SOCKET socket;
	unsigned int ID;
};

SOCKET createServerSocket(const int& PORT)
{
	//Initialize winsock
	WSADATA wsData; //Winsockdata
	WORD version = MAKEWORD(2, 2); //Winsockversion
	int wsOK = WSAStartup(version, &wsData); //Initialize
	if (wsOK != 0)
	{
		spdlog::error("Can't initialize winsock! Error: {}", wsOK);
		WSACleanup(); //Clear errormemory
		std::cin.get();
		return 1;
	}
	else
	{
		spdlog::info("Winsock initialized!");
	}

	//Create socket
	SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0); //af: AF_INIT := AdressFamily_InternetType := IPV4 | Type: SOCK_STREAM := TCP |
	if (serverSocket == INVALID_SOCKET)
	{
		int error = WSAGetLastError();
		spdlog::error("Can't create server socket! Error: {}", error);
		WSACleanup(); //Clear errormemory
		std::cin.get();
		return 1;
	}
	else
	{
		spdlog::info("Server socket created!");
	}

	//Fill in hintobject (IP, Port, Certificates)
	sockaddr_in socketObj = {};  //Create the object
	socketObj.sin_family = AF_INET; //Set AdressFamily
	socketObj.sin_port = htons(PORT); //Set Port
	socketObj.sin_addr.S_un.S_addr = INADDR_ANY; //Set Port for any IP

	//Bind hintobject to socket
	int bindOK = bind(serverSocket, (sockaddr*)&socketObj, sizeof(socketObj)); //Bind hint object to the socket
	if (bindOK == SOCKET_ERROR)
	{
		spdlog::error("Can't bind socket! Error: {}", bindOK);
		WSACleanup(); //Clear errormemory
		closesocket(serverSocket);
		std::cin.get();
		return 1;
	}
	else
	{
		spdlog::info("Socket bound!");
	}

	//Configure socket for listening
	int listenOK = listen(serverSocket, SOMAXCONN); //Maximum queue length
	if (listenOK == SOCKET_ERROR)
	{
		spdlog::error("Can't listen on socket! Error: {}", listenOK);
		WSACleanup(); //Clear errormemory
		closesocket(serverSocket);
		std::cin.get();
		return 1;
	}
	else
	{
		spdlog::info("Listen on socket!");
	}

	return serverSocket;
}

void ClientToClient(SocketStruct& RecvClient, std::vector<SocketStruct*>& Clients, char* ClientNameBuffer, unsigned int ClientID)
{
	char clientData[4096]; //Buffer for clientdata
	std::string dataString; //C++ Buffer
	unsigned int currentClientID = ClientID; //ID to keep track of current Client

	while (Clients.size() > 0)
	{
		ZeroMemory(clientData, 4096); //Clear buffer
		dataString.clear(); //Clear String

		//Check if TestMode is active
		if (TestMode == false)
		{
			//Wait for data from client
			int bytesReceived = recv(RecvClient.socket, clientData, 4096, 0);
			if (bytesReceived == SOCKET_ERROR)
			{
				int error = WSAGetLastError();
				spdlog::error("Error while receiving data! Error: {}", error);
				WSACleanup(); //Clear errormemory
				break;
			}

			//Convert data (C-String) into C++-String
			dataString = std::string(clientData);

			//Check for Modes
			if (dataString == "#DISC")
			{
				spdlog::warn("Client ({}) disconnected!", ClientNameBuffer);

				ClientMTX.lock();

				//Inform other clients about the disconnect
				for (SocketStruct* client : Clients)
				{
					//Send data to the other client
					int sendResult = send(client->socket, clientData, 4096, 0);
					if (sendResult == SOCKET_ERROR)
					{
						int error = WSAGetLastError();
						spdlog::error("Error while sending data! Error: {}", error);
						WSACleanup(); //Clear errormemory
						std::cin.get();
						break;
					}
				}

				//Copy vectordata
				std::vector<SocketStruct*> temp;
				for (int i = 0; i < Clients.size(); i++)
				{
					if (Clients.at(i)->ID != currentClientID)
					{
						temp.push_back(Clients.at(i));
					}
					else
					{
						spdlog::warn("Client wurde aus der Datenstruktur entfernt!");
					}
				}

				Clients.clear();
				Clients = temp;
				Clients.shrink_to_fit();

				ClientMTX.unlock();
				break;
			}
			else if (dataString == "#TEST")
			{
				firstTestClient = currentClientID;
				TestMode = true;
				spdlog::info("Testmode aktiviert!");

				ClientMTX.lock();
				//Send the message to ALL other clients
				for (int i = 0; i < Clients.size(); i++)
				{
					if (Clients.at(i)->ID != currentClientID && Clients.at(i)->ID != secondTestClient && Clients.at(i)->ID != firstTestClient)
					{
						int sendResult = send(Clients.at(i)->socket, clientData, 4096, 0);
						if (sendResult == SOCKET_ERROR)
						{
							int error = WSAGetLastError();
							spdlog::error("Error while sending data! Error: {}", error);
							WSACleanup(); //Clear errormemory
							std::cin.get();
							break;
						}
					}
				}
				ClientMTX.unlock();
			}
			else if (dataString == "#NORECV")
			{
				secondTestClient = currentClientID;
				spdlog::info("Zweiter Testclient wurde erkannt!");
			}
			else if (dataString == "#ALLRECV")
			{
				firstTestClient = 999999;
				secondTestClient = 999999;
				spdlog::info("Testclients wurden resettet!");
			}
			else
			{
				//Output the message
				spdlog::info("Client ({}): {}", ClientNameBuffer, clientData);

				ClientMTX.lock();
				//Send the message to ALL other clients
				for (int i = 0; i < Clients.size(); i++)
				{
					if (Clients.at(i)->ID != currentClientID && Clients.at(i)->ID != secondTestClient && Clients.at(i)->ID != firstTestClient)
					{
						int length = 4096;
						if(currentClientID == secondTestClient)
						{
							length = 6;
						}
						//std::cout << "Length: " << length << "\n";
						int sendResult = send(Clients.at(i)->socket, clientData, length, 0);
						if (sendResult == SOCKET_ERROR)
						{
							int error = WSAGetLastError();
							spdlog::error("Error while sending data! Error: {}", error);
							WSACleanup(); //Clear errormemory
							std::cin.get();
							break;
						}
					}
				}
				ClientMTX.unlock();
			}
		}
		else if (TestMode == true)
		{
			//Wait for data from client
			int bytesReceived = recv(RecvClient.socket, clientData, 6, 0);
			if (bytesReceived == SOCKET_ERROR)
			{
				int error = WSAGetLastError();
				spdlog::error("Error while receiving data! Error: {}", error);
				WSACleanup(); //Clear errormemory
				break;
			}

			//Convert data (C-String) into C++-String
			dataString = std::string(clientData); 

			//Check for Endstring
			if (dataString != "###")
			{
				//std::cout << clientData << "\r";

				ClientMTX.lock();
				//Send the message to ALL other clients
				for (int i = 0; i < Clients.size(); i++)
				{
					if (Clients.at(i)->ID != currentClientID && Clients.at(i)->ID != secondTestClient && Clients.at(i)->ID != firstTestClient)
					{
						int sendResult = send(Clients.at(i)->socket, clientData, 6, 0);
						if (sendResult == SOCKET_ERROR)
						{
							int error = WSAGetLastError();
							spdlog::error("Error while sending data! Error: {}", error);
							WSACleanup(); //Clear errormemory
							std::cin.get();
							break;
						}
					}
				}
				ClientMTX.unlock();
			}
			else
			{
				TestMode = false;
				spdlog::info("Testmode deaktiviert!");
				std::string dataString = "###";

				ClientMTX.lock();
				//Send the message to ALL other clients
				for (int i = 0; i < Clients.size(); i++)
				{
					if (Clients.at(i)->ID != currentClientID && Clients.at(i)->ID != secondTestClient && Clients.at(i)->ID != firstTestClient)
					{
						int sendResult = send(Clients.at(i)->socket, dataString.c_str(), dataString.size(), 0);
						if (sendResult == SOCKET_ERROR)
						{
							int error = WSAGetLastError();
							spdlog::error("Error while sending data! Error: {}", error);
							WSACleanup(); //Clear errormemory
							std::cin.get();
							break;
						}
					}
				}
				ClientMTX.unlock();
			}
		}
	}
}

int main()
{
	//Create server socket
	SOCKET listenerSocket = createServerSocket(54000);
	//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	//Controlstructures
	std::vector<SocketStruct*> Clients;
	std::vector<std::thread*> Threads;	
	unsigned int ClientID = 0;

	//Accept clients in a loop
	do
	{
		ClientID++;
		SocketStruct* SS = new SocketStruct();
		SS->socketObj = {};
		SS->socketSize = sizeof(SS->socketObj);
		SS->socket = accept(listenerSocket, (sockaddr*)&SS->socketObj, &SS->socketSize);
		SS->ID = ClientID;

		if (SS->socket == INVALID_SOCKET)
		{
			int error = WSAGetLastError();
			spdlog::error("Can't accept a client! Error: {}", error);
			WSACleanup(); //Clear errormemory
			std::cin.get();
			break;
		}
		else
		{
			spdlog::info("Client accepted!");
		}

		//Buffers for host-Data
		char hostBuffer[NI_MAXHOST]; //Client's remote name
		char infoBuffer[NI_MAXSERV]; //Service (i.e port) the client is connected on

		//Clear Buffers
		ZeroMemory(hostBuffer, NI_MAXHOST);
		ZeroMemory(infoBuffer, NI_MAXSERV);

		//Try to DNS-lookup the clientname
		if (getnameinfo((sockaddr*) & (SS->socketObj), SS->socketSize, hostBuffer, NI_MAXHOST, infoBuffer, NI_MAXSERV, 0) == 0)
		{
			spdlog::info("Client: {}", hostBuffer, " connected on Port: {}", infoBuffer);
		}
		else //otherwise show ClientIP
		{
			inet_ntop(AF_INET, &SS->socketObj.sin_addr, hostBuffer, NI_MAXHOST);
			auto clientIP = ntohs(SS->socketObj.sin_port);
			spdlog::info("Host: {}", hostBuffer, " with IP: {}", clientIP);
		}

		ClientMTX.lock();
		Clients.push_back(SS);
		std::thread* thread = new std::thread(ClientToClient, std::ref(*SS), std::ref(Clients), hostBuffer, ClientID);
		Threads.push_back(thread);
		ClientMTX.unlock();		
	}
	while (ClientID < 4);

	//Close listening socket
	closesocket(listenerSocket);

	//Wait for threads
	for(std::thread *thread : Threads)
	{
		thread->join();
	}

	//Close sockets	
	for(SocketStruct *client : Clients)
	{
		closesocket(client->socket);
	}

	//Clean up winsock
	WSACleanup();

	std::cin.get();
	return 0;
}