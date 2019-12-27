#include <iostream>
#include <WS2tcpip.h>
#include <string>
#include <spdlog/spdlog.h>
#include <thread>
#include <chrono>

#pragma comment(lib, "ws2_32.lib")

bool receiving = true; //Receiving-loop boolean
bool ChatOn = true; //Chat not closed boolean
bool TestMode = false; //Testmode active boolean
bool dataMissing = false; //To if data is missing after servercrash
long long sum = 0; //Variable for summing
unsigned int counter = 0; //To keep track of i in sendDataSlow
unsigned int oldState = 0; //To save the old state of counter

void sendDummyData(SOCKET& connect_socket)
{
	int i = 0;
	std::string data;
	do
	{
		data.clear();
		if(i < 10) 
			data = "00000" + std::to_string(i);
		else if(i < 100)
			data = "0000" + std::to_string(i);
		else if(i < 1000)
			data = "000" + std::to_string(i);
		else if (i < 10000)
			data = "00" + std::to_string(i);
		else if (i < 100000)
			data = "0" + std::to_string(i);
		else
			data = std::to_string(i);

		int sendResult = send(connect_socket, data.c_str(), data.length(), 0);
		if (sendResult == SOCKET_ERROR)
		{
			int error = WSAGetLastError();
			spdlog::error("Error while sending data! Error: {}", error);
			WSACleanup(); //Clear errormemory
			break;
		}
		i++;
	}
	while (i < 50000);
}

void sendDataSlow(SOCKET& connect_socket, int i)
{
	std::string data;
	do
	{
		data.clear();
		if (i < 10)
			data = "00000" + std::to_string(i);
		else if (i < 100)
			data = "0000" + std::to_string(i);
		else if (i < 1000)
			data = "000" + std::to_string(i);
		else if (i < 10000)
			data = "00" + std::to_string(i);
		else if (i < 100000)
			data = "0" + std::to_string(i);
		else
			data = std::to_string(i);

		int sendResult = send(connect_socket, data.c_str(), data.length(), 0);
		if (sendResult == SOCKET_ERROR)
		{
			int error = WSAGetLastError();
			spdlog::error("Error while sending data! Error: {}", error);
			WSACleanup(); //Clear errormemory
			break;
		}

		i++;
		counter = i;
		std::this_thread::sleep_for(std::chrono::milliseconds(1));

	} while (i < 10000);	
}

void receiveData(SOCKET& connect_socket)
{
	char serverData[4096]; //Buffer for clientdata		

	while (receiving == true)
	{
		ZeroMemory(serverData, 4096); //Clear buffer		

		//Check if TestMode is active
		if (TestMode == false)
		{
			int bytesReceived = recv(connect_socket, serverData, 4096, 0); //Receive (listen) on socket
			if (receiving == true)
			{
				if (bytesReceived == SOCKET_ERROR)
					{
						int error = WSAGetLastError();
						spdlog::error("Error while receiving data! Error: {}", error);
						WSACleanup(); //Clear errormemory
						break;
					}
				std::string dataString = std::string(serverData); //Convert the serverData (C-String) into a C++-String

				//Check for modes
				if (dataString == "#DISC")
					{
						spdlog::warn("Chat Partner ist disconnected!");
					}
				else if (dataString == "#TEST")
					{
						TestMode = true;
						spdlog::info("Testmode aktiviert!");
					}
				else
					{
						spdlog::info("Chat Partner: {}", serverData);
					}
			}
		}
		else if (TestMode == true)
		{
			int bytesReceived = recv(connect_socket, serverData, 6, 0); //Receive (listen) on socket
			if (bytesReceived == SOCKET_ERROR)
				{
					int error = WSAGetLastError();
					spdlog::error("Error while receiving data! Error: {}", error);
					WSACleanup(); //Clear errormemory
					break;
				}
			std::string dataString = std::string(serverData); //Convert the serverData (C-String) into a C++-String

			//Check for end
			if (dataString != "###")
			{
				sum += std::stoi(dataString); //Convert string into int to sum it up	
				std::cout << "Summe: " << sum << "\r";
			}
			else
			{
				spdlog::info("Ergebnis: {}", sum);
				TestMode = false;
				spdlog::info("Testmode deaktiviert!");
				sum = 0;
			}			
		}
	}
}

void sendData(SOCKET& connect_socket)
{
	std::string userInput; //Inputdata

	//Go into sending loop
	do
	{	
		//Wait for inputtext
		std::getline(std::cin, userInput); //Getline to have spaces and to finish a line with enter
		if (dataMissing == true)
		{
			userInput = "TEST";
		}
		if (userInput.size() > 0 || dataMissing == true)
		{	
			if (userInput == "#DISC")
			{
				int sendResult = send(connect_socket, userInput.c_str(), userInput.size(), 0); //Inform server about the disconnect
				if (sendResult == SOCKET_ERROR)
				{
					int error = WSAGetLastError();
					spdlog::error("Error while sending data! Error: {}", error);
					WSACleanup(); //Clear errormemory
					break;
				}
				receiving = false; //Quit receiving loop
				ChatOn = false; //Close out Chat
				spdlog::warn("Disconnected!");
				break;
			}
			else if (userInput == "#TEST")
			{
				spdlog::info("Selftestmode aktiviert!");
				int sendResult = send(connect_socket, userInput.c_str(), userInput.size(), 0); //Inform server about the test
				if (sendResult == SOCKET_ERROR)
				{
					int error = WSAGetLastError();
					spdlog::error("Error while sending data! Error: {}", error);
					WSACleanup(); //Clear errormemory
					break;
				}

				sendDummyData(connect_socket); //Create and send dummy data to the server

				userInput = "###"; //Create string to mark the end of the testmode
				sendResult = send(connect_socket, userInput.c_str(), userInput.size(), 0); //Send endstring to the server
				if (sendResult == SOCKET_ERROR)
				{
					int error = WSAGetLastError();
					spdlog::error("Error while sending data! Error: {}", error);
					WSACleanup(); //Clear errormemory
					break;
				}
				spdlog::info("Dummydata send! -> Selftestmode deaktiviert!");
			}
			else if (userInput == "#TEST1")
			{
				userInput = "#TEST";
				spdlog::info("Selftestmode aktiviert!");
				int sendResult = send(connect_socket, userInput.c_str(), userInput.size(), 0); //Inform server about the test
				if (sendResult == SOCKET_ERROR)
				{
					int error = WSAGetLastError();
					spdlog::error("Error while sending data! Error: {}", error);
					WSACleanup(); //Clear errormemory
					break;
				}

				sendDummyData(connect_socket); //Create and send dummy data to the server

				spdlog::info("Dummydata send! -> Selftestmode deaktiviert!");
			}
			else if (userInput == "#TEST2")
			{
				sendDummyData(connect_socket); //Create and send dummy data to the server

				userInput = "###"; //Create string to mark the end of the testmode
				int sendResult = send(connect_socket, userInput.c_str(), userInput.size(), 0); //Send endstring to the server
				if (sendResult == SOCKET_ERROR)
				{
					int error = WSAGetLastError();
					spdlog::error("Error while sending data! Error: {}", error);
					WSACleanup(); //Clear errormemory
					break;
				}
			}
			else if (userInput == "#SLOWTEST" || dataMissing == true)
			{
				std::string userInput = "#TEST";
				int sendResult = send(connect_socket, userInput.c_str(), userInput.size(), 0); //Inform server about the test
				if (sendResult == SOCKET_ERROR)
				{
					int error = WSAGetLastError();
					spdlog::error("Error while sending data! Error: {}", error);
					WSACleanup(); //Clear errormemory
				}

				sendDataSlow(connect_socket, oldState);

				userInput = "###"; //Create string to mark the end of the testmode
				sendResult = send(connect_socket, userInput.c_str(), userInput.size(), 0); //Send endstring to the server
				if (sendResult == SOCKET_ERROR)
				{
					int error = WSAGetLastError();
					spdlog::error("Error while sending data! Error: {}", error);
					WSACleanup(); //Clear errormemory
				}

				dataMissing = false;
				oldState = 0;
			}
			else
			{				
				int sendResult = send(connect_socket, userInput.c_str(), userInput.size(), 0); //Send inputdata to the server
				if (sendResult == SOCKET_ERROR)
				{
					int error = WSAGetLastError();
					spdlog::error("Error while sending data! Error: {}", error);
					WSACleanup(); //Clear errormemory
					break;
				}
			}
		}
	} while (userInput.size() > 0 && dataMissing == false);
}

SOCKET createSocket(const std::string& ServerIP, const int& port)
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
	SOCKET connect_socket = socket(AF_INET, SOCK_STREAM, 0); //af: AF_INIT := AdressFamily_InternetType := IPV4 | Type: SOCK_STREAM := TCP |
	if (connect_socket == INVALID_SOCKET)
	{
		int error = WSAGetLastError();
		spdlog::error("Can't create connect socket! Error: {}", error);
		WSACleanup(); //Clear errormemory
		std::cin.get();
		return 1;
	}
	else
	{
		spdlog::info("Connect socket created!");
	}

	//Fill in the socketobject
	sockaddr_in connectObj = {}; //Create the object
	connectObj.sin_family = AF_INET; //Set AdressFamily
	connectObj.sin_port = htons(port); //Set Port (host_to_network_short)
	int convertIP = inet_pton(AF_INET, ServerIP.c_str(), &connectObj.sin_addr); //Convert the string into "real" IP
	if (convertIP != 1)
	{
		spdlog::error("Can't convert IP-Adress! Error: {}", convertIP);
		WSACleanup(); //Clear errormemory
		closesocket(connect_socket);
		std::cin.get();
		return 1;
	}
	else
	{
		spdlog::info("IP converted!");
	}

	//Connect to server
	int connectOK = connect(connect_socket, (sockaddr*)&connectObj, sizeof(connectObj));
	if (connectOK == SOCKET_ERROR)
	{
		spdlog::error("Can't connect to server! Error: {}", connectOK);
		WSACleanup(); //Clear errormemory
		closesocket(connect_socket);
		std::cin.get();
		return 1;
	}
	else
	{
		spdlog::info("Connected to server!");
	}

	std::cout << "\n";
	return connect_socket;
}

int main()
{
	//Create Socket
	SOCKET connect_socket = createSocket("127.0.0.1", 54000);

	//Start threads
	std::thread t1(receiveData, std::ref(connect_socket));
	std::thread t2(sendData, std::ref(connect_socket));

	//Wait for sending thread
	t2.join();

	//Shutdown the connections
	int iResult = shutdown(connect_socket, SD_RECEIVE);
	if (iResult == SOCKET_ERROR) {
		spdlog::error("Receiving shutdown failed! Error: {}", iResult);
		closesocket(connect_socket);
		WSACleanup();
	}
	iResult = shutdown(connect_socket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		spdlog::error("Sending shutdown failed! Error: {}", iResult);
		closesocket(connect_socket);
		WSACleanup();
	}

	//Close socket
	closesocket(connect_socket);

	//Clean up winsock
	WSACleanup();

	//Wait for receiving thread
	t1.join();	

	if(ChatOn == false)
	{
		spdlog::info("Chat beendet. Enter schliesst Konsole!");
		std::cin.get();
		return 0;
	}
	else
	{
		//std::cin.get();

		//Create Socket
		SOCKET connect_socket = createSocket("127.0.0.1", 54001);		

		receiving = true;
		ChatOn = true;
		TestMode = false;

		std::cout << "Summe: " << sum << "\n";
		std::cout << "Counter: " << counter << "\n";		
		oldState = counter;

		if(counter > 0 && counter < 10000)
		{
			std::cout << "Senden..." << "\n";
			dataMissing = true;
		}
		else
		{
			std::cout << "Empfangen..." << "\n";			
		}

		//Start threads
		std::thread t1(receiveData, std::ref(connect_socket));
		std::thread t2(sendData, std::ref(connect_socket));

		//Wait for sending thread
		t2.join();

		//Shutdown the connections
		int iResult = shutdown(connect_socket, SD_RECEIVE);
		if (iResult == SOCKET_ERROR) {
			spdlog::error("Receiving shutdown failed! Error: {}", iResult);
			closesocket(connect_socket);
			WSACleanup();
		}
		iResult = shutdown(connect_socket, SD_SEND);
		if (iResult == SOCKET_ERROR) {
			spdlog::error("Sending shutdown failed! Error: {}", iResult);
			closesocket(connect_socket);
			WSACleanup();
		}

		//Close socket
		closesocket(connect_socket);

		//Clean up winsock
		WSACleanup();

		//Wait for receiving thread
		t1.join();

		spdlog::info("Chat beendet. Enter schliesst Konsole!");
		std::cin.get();
		return 0;
	}	
}