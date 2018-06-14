#ifdef __linux__

/*Header Files*/
#include <cstdlib>
#include <stdio.h>
#include<iostream>
#include <unistd.h>
#include<string.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netdb.h>
#include<fstream>

/*This class contains variables, accessible to all the client functions*/
using namespace std;
class client {
private:
	int sock;
	std::string address;
	int port;
	struct sockaddr_in server;

public:
	client();
	bool conn(string, int);
	bool send_data(string command, string filePath, string data);
	string receive(int);

};
client::client()
{
	sock = -1;
	port = 0;
	address = "";
}

/**
*
* @param address--client IP address, passed from main();
* @param port--Port info from the main()
* @return boolean value
*/
bool client::conn(string address, int port)
{
	if (sock == -1)
	{
		//Create socket
		sock = socket(AF_INET, SOCK_STREAM, 0);
		if (sock == -1)
		{
			perror("Could not create socket");
			exit(0);
		}
		cout << "Socket created\n";
	}
	else {   /* OK , nothing */ }
	//setting up address structure
	if (inet_addr(address.c_str()) == -1)
	{
		struct hostent *hostEntity;
		struct in_addr **addr_list;
		//resolves the host name;handles the errors
		if ((hostEntity = gethostbyname(address.c_str())) == NULL)
		{
			//gethostbyname failed
			herror("gethostbyname");
			cout << "Failed to resolve hostname\n";
			exit(0);
			return false;
		}

		//Cast the h_addr_list to in_addr , since h_addr_list also has the IP address in long format
		addr_list = (struct in_addr **) hostEntity->h_addr_list;
		for (int i = 0; addr_list[i] != NULL; i++)
		{
			server.sin_addr = *addr_list[i];
			cout << address << " resolved to " << inet_ntoa(*addr_list[i]) << endl;
			break;
		}
	}
	//In case of a correct IP address format
	else
	{
		server.sin_addr.s_addr = inet_addr(address.c_str());
	}
	server.sin_family = AF_INET;
	server.sin_port = htons(port);

	//Connect to remote server and handle the error, if any
	if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0)
	{
		perror("connect failed. Error");
		close(sock);
		exit(0);
		return 1;
	}
	cout << "Connected\n";
	return true;
}

/**
* @param command--GET or PUT; from main()
* @param file--the file to be passed to the server; from main()
* @param protocol--HTTP/1.1 protocol
* @return boolean value
*/
bool client::send_data(string command, string file, string protocol)
{
	string host = "127.0.0.1:8000";
	//If command is GET, execute this function
	if (!command.compare("GET"))
	{
		string data = command + " /" + file + " " + protocol;

		//send all the basic client details to server through the socket
		if (send(sock, data.c_str(), strlen(data.c_str()), 0)<0)
		{
			perror("Send failed : ");
			close(sock);
			exit(0);
			return false;
		}
		cout << "Data sent\n";
	}

	//If command is PUT, execute this function
	else if (!command.compare("POST"))
	{
		/*concatenate the passed parameters to "data" variable and use
		*input file stream object to read the corresponding file*/
		string data = command + " /server.txt " + protocol;
		ifstream openfile(file.c_str());
		string getcontent = "";

		/*If the file is present*/
		if (openfile.is_open())
		{
			//stays inside the loop till the end of the file is reached;
			//getline() function gets characters till a CRLF is met in 
			//the file--in short gets each line of the file
			while (!openfile.eof())
			{
				getline(openfile, getcontent);
				cout << getcontent << endl;
				data = data + getcontent;
			}

			//send the file data through the socket and handle the error, if any
			if (send(sock, data.c_str(), strlen(data.c_str()), 0) < 0)
			{
				perror("Sending failed : ");
				close(sock);
				exit(0);
				return false;
			}
			cout << "Data sent\n";
		}
		else
		{
			cout << "File Not Found";
			close(sock);
			exit(0);
		}
	}
	return true;
}

/*
* This function receives the response from the server
*and puts them in the buffer for display
*--hence the function returns string
*/
string client::receive(int size = 512)
{
	char buffer[size];
	string reply;
	//Receive a reply from the server
	if (recv(sock, buffer, sizeof(buffer), 0) < 0)
	{
		puts("Reception failed");
	}
	reply = buffer;
	return reply;
}

/**
* main() function of client
* @param argc--takes integer value; gives the number of arguments
* @param argv--double character pointer--has five parameters
* executable file
* Server IP address--client IP address
* Port Number--should correlate with the server port to make a successful connection
* Command--GET or PUT
* File--file to send to server
* @return integer value
*/
int main(int argc, char** argv)
{
	//Check the number of parameters
	if (argc < 5)
	{
		fprintf(stderr, "ERROR, One or more of 4 parameters missing!\n");
		exit(1);
	}
	//assign the parameters to local variables
	string serverName = argv[1];
	int port = atoi(argv[2]);
	string command = argv[3];
	string filePath = argv[4];
	client c;
	cout << "First argument: " << serverName << endl;
	cout << "Second argument: " << port << endl;
	cout << "Third argument: " << command << endl;
	cout << "Fourth argument: " << filePath << endl;
	//connect to host
	c.conn(serverName, port);
	//send some data
	c.send_data(command, filePath, "HTTP/1.1\r\n\r\n");
	//receive and echo reply
	cout << "----------------------------\n\n";
	cout << c.receive(1024);
	cout << "\n\n----------------------------\n\n";
	return 0;
}


#elif _WIN32 | _WIN64

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <iostream>


using namespace std;

// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#define DEFAULT_BUFLEN 512

string host = "127.0.0.1:8000";

void showMenu()
{
	printf("1.Send GET request\n");
	printf("2.Send PUT request\n");
}

int sendGetRequest(SOCKET sock) {
	string page;
	cout << "Input page: ";
	cin >> page;
	string sendbuf = "GET /" + page + " HTTP/1.1\r\nHost: " + host + "\r\nConnection: close\r\n\r\n";
	return send(sock, sendbuf.c_str(), (int)strlen(sendbuf.c_str()), 0);
}
int sendPutRequest(SOCKET sock) {
	string dataToSend;
	cout << "Enter data to send to server: " << endl;
	cin >> dataToSend;
	string sendbuf = "PUT /server.txt HTTP/1.1\r\nHost: " + host +
		"Content-Length: " + to_string(strlen(dataToSend.c_str())) + "\r\n\r\n" +
		dataToSend;
	return send(sock, sendbuf.c_str(), (int)strlen(sendbuf.c_str()), 0);
}

int __cdecl main(int argc, char **argv)
{
	WSADATA wsaData;
	SOCKET ConnectSocket = INVALID_SOCKET;
	struct addrinfo *result = NULL,
		*ptr = NULL,
		hints;
	char recvbuf[DEFAULT_BUFLEN];
	int iResult;
	int recvbuflen = DEFAULT_BUFLEN;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// Resolve the server address and port
	iResult = getaddrinfo("127.0.0.1", "8000", &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	// Attempt to connect to an address until one succeeds

	int sym;
	int choose;
	do {
		for (ptr = result; ptr != NULL;ptr = ptr->ai_next) {

			// Create a SOCKET for connecting to server
			ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
				ptr->ai_protocol);
			if (ConnectSocket == INVALID_SOCKET) {
				printf("socket failed with error: %ld\n", WSAGetLastError());
				WSACleanup();
				return 1;
			}

			// Connect to server.
			iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
			if (iResult == SOCKET_ERROR) {
				closesocket(ConnectSocket);
				ConnectSocket = INVALID_SOCKET;
				continue;
			}
			break;
		}

		//freeaddrinfo(result);

		if (ConnectSocket == INVALID_SOCKET) {
			printf("Unable to connect to server!\n");
			WSACleanup();
			system("pause"); 
			return 1;
		}

		showMenu();
		fflush(stdin);
		scanf_s("%d", &choose);

		switch (choose) {
		case 1: iResult = sendGetRequest(ConnectSocket);
			break;
		case 2: iResult = sendPutRequest(ConnectSocket);
			break;
		}

		printf("Bytes Sent: %ld\n", iResult);

		// shutdown the connection since no more data will be sent
		iResult = shutdown(ConnectSocket, SD_SEND);
		if (iResult == SOCKET_ERROR) {
			printf("shutdown failed with error: %d\n", WSAGetLastError());
			closesocket(ConnectSocket);
			WSACleanup();
			return 1;
		}

		// Receive until the peer closes the connection
		do {

			iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
			if (iResult > 0) {
				//printf("Bytes received: %d\n", iResult);
				for (int i = 0; i < iResult; i++) {
					printf("%c", recvbuf[i]);
				}
			}
			else if (iResult == 0)
				printf("\nConnection closed\n");
			else
				printf("recv failed with error: %d\n", WSAGetLastError());

		} while (iResult > 0);


		// cleanup
		closesocket(ConnectSocket);
		printf("\nDo you want to send one more request?? (0 - NO)  ");
		fflush(stdin);
		scanf_s("%d", &sym);
		system("cls");
	} while (sym != 0);
	WSACleanup();

	system("pause");

	return 0;
}
#endif
