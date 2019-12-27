#include <WS2tcpip.h>
#include <iostream>
#include <spdlog/spdlog.h>
#include <thread>
#include <string>

#pragma comment(lib, "ws2_32.lib")

static bool receiving = true;
static bool TestMode = false;

static struct SocketStruct
{
	sockaddr_in socketObj;
	int socketSize;
	SOCKET socket;
};

static SOCKET createServerSocket(const int& PORT)
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

static SocketStruct createClientSocket(SOCKET& serverSocket)
{
	SocketStruct SS;

	SS.socketObj = {};
	SS.socketSize = sizeof(SS.socketObj);
	SS.socket = accept(serverSocket, (sockaddr*)&SS.socketObj, &SS.socketSize);
	if (SS.socket == INVALID_SOCKET)
	{
		int error = WSAGetLastError();
		spdlog::error("Can't accept a client! Error: {}", error);
		WSACleanup(); //Clear errormemory
		std::cin.get();
		return SS;
	}
	else
	{
		spdlog::info("Client accepted!");
	}

	return SS;
}

static void ClientToClient(SocketStruct& recvClient, SocketStruct& sendClient_1, SocketStruct& sendClient_2, char* clientNameBuffer)
{
	char clientData[4096]; //Buffer for clientdata	

	while (receiving == true)
	{
		ZeroMemory(clientData, 4096); //Clear buffer

		//Check if TestMode is active
		if (TestMode == false) 
		{
			//Wait for data from client
			int bytesReceived = recv(recvClient.socket, clientData, 4096, 0);
			if (bytesReceived == SOCKET_ERROR)
			{
				int error = WSAGetLastError();
				spdlog::error("Error while receiving data! Error: {}", error);
				WSACleanup(); //Clear errormemory
				break;
			}

			//Convert data (C-String) into C++-String
			std::string dataString = std::string(clientData);

			//Check for modes
			if (dataString == "#DISC")
			{
				int sendResult = send(sendClient_1.socket, clientData, 4096, 0);
				if (sendResult == SOCKET_ERROR)
				{
					int error = WSAGetLastError();
					spdlog::error("Error while sending data! Error: {}", error);
					WSACleanup(); //Clear errormemory
					std::cin.get();
					break;
				}
				
				int sendResult_2 = send(sendClient_2.socket, clientData, 4096, 0);
				if (sendResult_2 == SOCKET_ERROR)
				{
					int error = WSAGetLastError();
					spdlog::error("Error while sending data! Error: {}", error);
					WSACleanup(); //Clear errormemory
					std::cin.get();
					break;
				}
				
				spdlog::warn("Client ({}) disconnected!", clientNameBuffer);
				receiving = false;
				break;
			}
			else if (dataString == "#TEST")
			{
				TestMode = true;
				spdlog::info("Testmode aktiviert!");
				int sendResult = send(sendClient_1.socket, clientData, 4096, 0);
				if (sendResult == SOCKET_ERROR)
				{
					int error = WSAGetLastError();
					spdlog::error("Error while sending data! Error: {}", error);
					WSACleanup(); //Clear errormemory
					std::cin.get();
					break;
				}
				
				int sendResult_2 = send(sendClient_2.socket, clientData, 4096, 0);
				if (sendResult_2 == SOCKET_ERROR)
				{
					int error = WSAGetLastError();
					spdlog::error("Error while sending data! Error: {}", error);
					WSACleanup(); //Clear errormemory
					std::cin.get();
					break;
				}
			}
			else
			{
				//Output the message
				spdlog::info("Client ({}): {}", clientNameBuffer, clientData);

				//Send data to the other client
				int sendResult = send(sendClient_1.socket, clientData, 4096, 0);
				if (sendResult == SOCKET_ERROR)
				{
					int error = WSAGetLastError();
					spdlog::error("Error while sending data! Error: {}", error);
					WSACleanup(); //Clear errormemory
					std::cin.get();
					break;
				}
				
				int sendResult_2 = send(sendClient_2.socket, clientData, 4096, 0);
				if (sendResult_2 == SOCKET_ERROR)
				{
					int error = WSAGetLastError();
					spdlog::error("Error while sending data! Error: {}", error);
					WSACleanup(); //Clear errormemory
					std::cin.get();
					break;
				}
			}
		}
		else if (TestMode == true)
		{
			int bytesReceived = recv(recvClient.socket, clientData, 6, 0); //Receive (listen) on socket
			if (bytesReceived == SOCKET_ERROR)
			{
				int error = WSAGetLastError();
				spdlog::error("Error while receiving data! Error: {}", error);
				WSACleanup(); //Clear errormemory
				break;
			}

			std::string dataString = std::string(clientData); //Convert the serverData (C-String) into a C++-String

			//Check for end
			if (dataString != "###")
			{
				//std::cout << clientData << "\r";
				int sendResult = send(sendClient_1.socket, clientData, 6, 0);
				if (sendResult == SOCKET_ERROR)
				{
					int error = WSAGetLastError();
					spdlog::error("Error while sending data! Error: {}", error);
					WSACleanup(); //Clear errormemory
					std::cin.get();
					break;
				}
				
				int sendResult_2 = send(sendClient_2.socket, clientData, 6, 0);
				if (sendResult_2 == SOCKET_ERROR)
				{
					int error = WSAGetLastError();
					spdlog::error("Error while sending data! Error: {}", error);
					WSACleanup(); //Clear errormemory
					std::cin.get();
					break;
				}
			}
			else
			{
				TestMode = false;
				spdlog::info("Testmode deaktiviert!");
				std::string dataString = "###";

				int sendResult = send(sendClient_1.socket, dataString.c_str(), dataString.size(), 0); //Inform client about the end of the testmode
				if (sendResult == SOCKET_ERROR)
				{
					int error = WSAGetLastError();
					spdlog::error("Error while sending data! Error: {}", error);
					WSACleanup(); //Clear errormemory
					break;
				}
				
				int sendResult_2 = send(sendClient_2.socket, dataString.c_str(), dataString.size(), 0); //Inform client about the end of the testmode
				if (sendResult_2 == SOCKET_ERROR)
				{
					int error = WSAGetLastError();
					spdlog::error("Error while sending data! Error: {}", error);
					WSACleanup(); //Clear errormemory
					break;
				}
			}
		}
	}
}

int old_main_dont_use()
{
	//Create server socket
	SOCKET listenerSocket = createServerSocket(54000);	
	//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	//Host_1
	SocketStruct client_1 = createClientSocket(listenerSocket);

	//Buffers for host-Data
	char hostBuffer_1[NI_MAXHOST]; //Client's remote name
	char infoBuffer_1[NI_MAXSERV]; //Service (i.e port) the client is connected on

	//Clear Buffers
	ZeroMemory(hostBuffer_1, NI_MAXHOST); 
	ZeroMemory(infoBuffer_1, NI_MAXSERV);

	//Try to DNS-lookup the clientname
	if (getnameinfo((sockaddr*)&(client_1.socketObj), client_1.socketSize, hostBuffer_1, NI_MAXHOST, infoBuffer_1, NI_MAXSERV, 0) == 0)
	{
		spdlog::info("Client: {}", hostBuffer_1, " connected on Port: {}", infoBuffer_1);
	}
	else //otherwise show ClientIP
	{
		inet_ntop(AF_INET, &client_1.socketObj.sin_addr, hostBuffer_1, NI_MAXHOST);
		auto clientIP = ntohs(client_1.socketObj.sin_port);
		spdlog::info("Host: {}", hostBuffer_1, " with IP: {}", clientIP);
	}
	//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	//Host_2
	SocketStruct client_2 = createClientSocket(listenerSocket);

	//Buffers for host-Data
	char hostBuffer_2[NI_MAXHOST]; //Client's remote name
	char infoBuffer_2[NI_MAXSERV]; //Service (i.e port) the client is connected on

	//Clear Buffers
	ZeroMemory(hostBuffer_2, NI_MAXHOST);
	ZeroMemory(infoBuffer_2, NI_MAXSERV);

	//Try to DNS-lookup the clientname
	if (getnameinfo((sockaddr*) & (client_2.socketObj), client_2.socketSize, hostBuffer_2, NI_MAXHOST, infoBuffer_2, NI_MAXSERV, 0) == 0)
	{
		spdlog::info("Client: {}", hostBuffer_2, " connected on Port: {}", infoBuffer_2);
	}
	else //otherwise show ClientIP
	{
		inet_ntop(AF_INET, &client_2.socketObj.sin_addr, hostBuffer_2, NI_MAXHOST);
		auto clientIP = ntohs(client_2.socketObj.sin_port);
		spdlog::info("Host: {}", hostBuffer_2, " with IP: {}", clientIP);
	}
	//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	//Host_3
	SocketStruct client_3 = createClientSocket(listenerSocket);

	//Buffers for host-Data
	char hostBuffer_3[NI_MAXHOST]; //Client's remote name
	char infoBuffer_3[NI_MAXSERV]; //Service (i.e port) the client is connected on

	//Clear Buffers
	ZeroMemory(hostBuffer_3, NI_MAXHOST);
	ZeroMemory(infoBuffer_3, NI_MAXSERV);

	//Try to DNS-lookup the clientname
	if (getnameinfo((sockaddr*) & (client_3.socketObj), client_3.socketSize, hostBuffer_3, NI_MAXHOST, infoBuffer_3, NI_MAXSERV, 0) == 0)
	{
		spdlog::info("Client: {}", infoBuffer_3, " connected on Port: {}", infoBuffer_3);
	}
	else //otherwise show ClientIP
	{
		inet_ntop(AF_INET, &client_3.socketObj.sin_addr, hostBuffer_3, NI_MAXHOST);
		auto clientIP = ntohs(client_3.socketObj.sin_port);
		spdlog::info("Host: {}", hostBuffer_3, " with IP: {}", clientIP);
	}
	//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	//Close listening socket
	closesocket(listenerSocket);

	std::cout << "\n";

	//Thread 1: Accept message from client_1 and send it to client_2 and client_3
	std::thread t1(ClientToClient, std::ref(client_1), std::ref(client_2), std::ref(client_3), hostBuffer_1);
	//Thread 2: Accept message from client_2 and send it to client_1 and client_2
	std::thread t2(ClientToClient, std::ref(client_2), std::ref(client_1), std::ref(client_3), hostBuffer_2);
	//Thread 3: Accept message from client_3 and send it to client_1
	std::thread t3(ClientToClient, std::ref(client_3), std::ref(client_1), std::ref(client_2), hostBuffer_3);
	
	//Wait for threads
	t1.join();
	t2.join();
	t3.join();

	//Close socket
	closesocket(client_1.socket);
	closesocket(client_2.socket);
	closesocket(client_3.socket);

	//Clean up winsock
	WSACleanup();

	std::cin.get();
	return 0;
}