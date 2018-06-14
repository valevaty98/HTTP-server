#ifdef __linux__

/*Header Files*/
#include <cstdlib>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include<iostream>
#include<sstream>
#include<fstream>
#include<csignal>
using namespace std;
int sockfd, newsockfd;

/*Function for displaying corresponding error message*/
void error(const char *msg)
{
	perror(msg);
	exit(1);
}

/**
* @param argc-- takes integer value; gives the number of arguments
* @param argv-- takes char pointer; server takes one argument: PORT Number
* @return integer
*/
int main(int argc, char *argv[])
{
	/*Variable Declaration*/
	int portNumber;
	socklen_t clientLength;
	char buffer[1024];
	struct sockaddr_in serverAddr, clientAddr;
	int n;

	/*checks for the PORT number argument*/
	if (argc < 2) {
		fprintf(stderr, "ERROR, no port provided\n");
		exit(1);
	}

	/*
	* Create Internet domain socket
	*  and checks for error in creating the socket
	*/
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
		error("ERROR opening socket");

	//Fills the entire socket structure with Zeroes
	bzero((char *)&serverAddr, sizeof(serverAddr));
	portNumber = atoi(argv[1]);

	/*
	* sets the values of socket structure members
	* and binds them with a defined socket
	*/
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddr.sin_port = htons(portNumber);
	if (bind(sockfd, (struct sockaddr *) &serverAddr,
		sizeof(serverAddr)) < 0)
		error("ERROR in binding");
	cout << "\nListening on port " << portNumber << " \n";

	/*The server listens for incoming connections with maximum limit upto 5 clients*/
	listen(sockfd, 5);
	do {
		clientLength = sizeof(clientAddr);
		newsockfd = accept(sockfd,
			(struct sockaddr *) &clientAddr,
			&clientLength);
		if (newsockfd < 0)
			error("ERROR on accept");
		bzero(buffer, 1024);

		/*reads the first information on the socket,
		which contains client-side details*/
		n = read(newsockfd, buffer, 1023);
		buffer[n] = 0;
		string request(buffer);
		if (n < 0)
			error("ERROR reading from socket");

		/*Declare a istringstream object for
		*dealing with string variables
		*and assign them to separate local string
		*variables
		*/
		const char *head = buffer;
		const char *tail = buffer;
		//buffer[1024] = '\0';
		string method;
		while (*tail != ' ') ++tail;
		method = std::string(head, tail);

		istringstream iss(request);
		int tmpParam = 0;
		string command, file, protocol;
		string dataStream = "";
		while (iss)
		{
			string word;
			iss >> word;
			//cout << word << endl;
			if (tmpParam == 0)
			{
				command = word;
			}
			else if (tmpParam == 1)
			{
				file = word;
			}
			else if (tmpParam == 2)
			{
				protocol = word;
			}
			else if (tmpParam>2)
			{
				dataStream = dataStream + " " + word;
			}
			tmpParam++;
		}
		dataStream += '\n';
		file = file.erase(0, 1);
		cout << command << endl;
		cout << file << endl;
		cout << protocol << endl;
		//When GET is selected from Client
		if (method == "GET")
		{
			string getcontent = "";
			string writeFileContents = "";
			ifstream openfile(file.c_str());
			if (openfile.is_open())
			{
				//cout << "\nYes, the file: " << file << " exists in the Server\n";
				writeFileContents = protocol + " 200 OK\r\nContent - Type: text / html\r\n\r\n";
				//writeFileContents += "File:" + file + " exists in the Server\n\n";

				//Passes server response till the end of file is met
				while (!openfile.eof())
				{
					getline(openfile, getcontent);
					// cout<<getcontent<<"\n";
					writeFileContents = writeFileContents + getcontent;
				}
				write(newsockfd, writeFileContents.c_str(), strlen(writeFileContents.c_str()));
			}
			else
			{
				//cout << "\nFile not found on Server";
				write(newsockfd, "404 Not Found", strlen("404 Not Found"));
			}
		} //When PUT is selected from Client
		else
		{
			/*Declare a output file stream to write the socket data to a file*/
			ofstream myfile;
			myfile.open(file.c_str(), ios_base::app);
			cout << "Writing content to file.\n";
			myfile << dataStream;

			//Closes the file and sends response to the client about file creation
			myfile.close();
			write(newsockfd, "200 OK", strlen("200 OK"));
		}
		//closes all the open sockets

		cout << "\nConnection closed." << endl;
		close(newsockfd);
	} while (1);
	close(sockfd);
	return 0;
}

#elif _WIN32 | _WIN64
#include <iostream>
#include <sstream>
#include <string>
#include <process.h>
#include <memory>
#include <thread>
#include <stdio.h>
#include <iostream>
#include <string>
#include <regex>
#include <vector>
#include <mutex>
#include <chrono>
#include <fstream>
#include <time.h>

//using namespace std;
// Для корректной работы freeaddrinfo в MinGW
// Подробнее: http://stackoverflow.com/a/20306451
#define _WIN32_WINNT 0x501

using std::cin;		using std::cout;
using std::endl;	using std::string;
using std::regex;   using std::regex_token_iterator;
using std::unique_ptr;
using std::make_unique;
using std::thread;
using std::vector;
using std::ofstream;
using std::ios_base;

#include <WinSock2.h>
#include <WS2tcpip.h>

// Необходимо, чтобы линковка происходила с DLL-библиотекой 
// Для работы с сокетам
#pragma comment(lib, "Ws2_32.lib")

using std::cerr;

void KeepSessionWithClient(int client_socket);
void openFileWithPathAndSend(string filePath, SOCKET clientInstance);
void sendBodyToServer(string body, SOCKET clientInstance);
void sendFile(FILE* m_file, SOCKET clientInstance);
int processMethod(SOCKET client_socket);
string processGet(char* bufferPtr);
string processPut(char* bufferPtr);
string getFilePath(string p_toParse);

int main()
{

	WSADATA wsaData;

	// старт использования библиотеки сокетов процессом
	// (подгружается Ws2_32.dll)
	int result = WSAStartup(MAKEWORD(2, 2), &wsaData);

	// Если произошла ошибка подгрузки библиотеки
	if (result != 0) {
		cerr << "WSAStartup failed: " << result << "\n";
		return result;
	}

	struct addrinfo* addr = NULL; // структура, хранящая информацию
								  // об IP-адресе  слущающего сокета

								  // Шаблон для инициализации структуры адреса
	struct addrinfo hints;
	ZeroMemory(&hints, sizeof(hints));

	// AF_INET определяет, что используется сеть для работы с сокетом
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM; // Задаем потоковый тип сокета
	hints.ai_protocol = IPPROTO_TCP; // Используем протокол TCP
									 // Сокет биндится на адрес, чтобы принимать входящие соединения
	hints.ai_flags = AI_PASSIVE;

	// Инициализируем структуру, хранящую адрес сокета - addr.
	// HTTP-сервер будет висеть на 8000-м порту локалхоста
	result = getaddrinfo("127.0.0.1", "8000", &hints, &addr);

	// Если инициализация структуры адреса завершилась с ошибкой,
	// выведем сообщением об этом и завершим выполнение программы 
	if (result != 0) {
		cerr << "getaddrinfo failed: " << result << "\n";
		WSACleanup(); // выгрузка библиотеки Ws2_32.dll
		return 1;
	}

	// Создание сокета
	int listen_socket = socket(addr->ai_family, addr->ai_socktype,
		addr->ai_protocol);
	// Если создание сокета завершилось с ошибкой, выводим сообщение,
	// освобождаем память, выделенную под структуру addr,
	// выгружаем dll-библиотеку и закрываем программу
	if (listen_socket == INVALID_SOCKET) {
		cerr << "Error at socket: " << WSAGetLastError() << "\n";
		freeaddrinfo(addr);
		WSACleanup();
		return 1;
	}
	// Привязываем сокет к IP-адресу
	result = bind(listen_socket, addr->ai_addr, (int)addr->ai_addrlen);
	// Если привязать адрес к сокету не удалось, то выводим сообщение
	// об ошибке, освобождаем память, выделенную под структуру addr.
	// и закрываем открытый сокет.
	// Выгружаем DLL-библиотеку из памяти и закрываем программу.
	if (result == SOCKET_ERROR) {
		cerr << "bind failed with error: " << WSAGetLastError() << "\n";
		freeaddrinfo(addr);
		closesocket(listen_socket);
		WSACleanup();
		return 1;
	}
	// Инициализируем слушающий сокет
	if (listen(listen_socket, SOMAXCONN) == SOCKET_ERROR) {
		cerr << "listen failed with error: " << WSAGetLastError() << "\n";
		closesocket(listen_socket);
		WSACleanup();
		return 1;
	}
	while (1)
	{
		// Принимаем входящие соединения
		int client_socket = accept(listen_socket, NULL, NULL);
		if (client_socket == INVALID_SOCKET) {
			cerr << "accept failed: " << WSAGetLastError() << "\n";
			closesocket(listen_socket);
			WSACleanup();
			return 1;
		}

		CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)KeepSessionWithClient, (LPVOID)client_socket, NULL, NULL);
	}

	// Убираем за собой
	closesocket(listen_socket);
	freeaddrinfo(addr);
	WSACleanup();
	return 0;
}

void KeepSessionWithClient(int client_socket) {
	cerr << "start to keep session " << client_socket << "\n";

	processMethod(client_socket);
}

string checkMethod(char *buffer, int recvMsgSize) {
	const char *head = buffer;
	const char *tail = buffer;
	buffer[recvMsgSize] = '\0';
	while (*tail != ' ') ++tail;
	return std::string(head, tail);
}

int processMethod(SOCKET clientInstance) {
	char buffer[1024];
	int recvMsgSize;
	int bufError;
	string filePath = { "" };
	string bodyOfPut = { "" };
	do {

		recvMsgSize = recv(clientInstance, buffer, 1024, 0);
		if (recvMsgSize > 0)
		{
			string method = checkMethod(buffer, recvMsgSize);
		
			if (method == "GET") 
			{
				filePath = processGet(buffer);
				cout << "Extracted filename: " << filePath << endl; 

				openFileWithPathAndSend(filePath, clientInstance);
				//shutdown the connection since no more data will be sent
				bufError = shutdown(clientInstance, SD_SEND);
				if (bufError == SOCKET_ERROR) {
					wprintf(L"shutdown failed with error: %d\n", WSAGetLastError());
					closesocket(clientInstance);
					WSACleanup();
				}
			}
			else if (method == "PUT")
			{
				bodyOfPut = processPut(buffer);
				cout << "Extracted body: " << bodyOfPut << endl;

				sendBodyToServer(bodyOfPut, clientInstance);

				bufError = shutdown(clientInstance, SD_SEND);
				if (bufError == SOCKET_ERROR) {
					wprintf(L"shutdown failed with error: %d\n", WSAGetLastError());
					closesocket(clientInstance);
					WSACleanup();
				}
			}
			else
			{
				printf("Incorrect HTTP-method!!!\n");
			}

			bufError = shutdown(clientInstance, SD_SEND);
			if (bufError == SOCKET_ERROR) {
				wprintf(L"shutdown failed with error: %d\n", WSAGetLastError());
				closesocket(clientInstance);
				WSACleanup();
			}

		}
		else if (recvMsgSize == 0)
		{
			printf("Connection closed\n");
		}
		else
		{
			printf("recv failed: %d\n", WSAGetLastError());
		}

	} while (recvMsgSize > 0);

	bufError = shutdown(clientInstance, SD_RECEIVE);
	if (bufError == SOCKET_ERROR) {
		wprintf(L"shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(clientInstance);
		WSACleanup();
	}

	// close the socket
	bufError = closesocket(clientInstance);
	if (bufError == SOCKET_ERROR) {
		wprintf(L"close failed with error: %d\n", WSAGetLastError());
		WSACleanup();
	}

	return 0;
}

string processGet(char* bufferPtr)
{
	string firstLine = { "" };
	while (*bufferPtr != '\r') {
		firstLine += *bufferPtr;
		bufferPtr++;
	}
	cout << "Client request: " << firstLine << endl;
	return getFilePath(firstLine);
}

string processPut(char* bufferPtr)
{
	string body = { "" };
	while (*bufferPtr != '\0') {
		bufferPtr++;
	}
	while (*bufferPtr != '\n') {
		bufferPtr--;
	}

	bufferPtr++;
	while (*bufferPtr != '\0') {
		body += *bufferPtr;
		bufferPtr++;
	}
	return body;
}

void openFileWithPathAndSend(string filePath, SOCKET clientInstance)
{
	FILE* m_file;
	errno_t err;
	err = fopen_s(&m_file, filePath.c_str(), "r");

	if (err == 0)//if i found the file i can send it back to browser
	{
		cout << "The file :" << filePath << " was opened." << endl;
		sendFile(m_file, clientInstance);

	}
	else//i didnt find the file i have to send 404 page not found
	{
		std::stringstream response; // сюда будет записываться ответ клиенту
		std::stringstream response_body; // тело ответа


										 //// Формируем весь ответ вместе с заголовками
										 //response << "HTTP/1.1 200 OK\r\n"
										 //	<< "Version: HTTP/1.1\r\n"
										 //	<< "Content-Type: text/html; charset=utf-8\r\n"
										 //	<< "Content-Length: " << response_body.str().length()
										 //	<< "\r\n\r\n"
										 //	<< response_body.str();

		response_body << "<title>Not Found file</title>\n"
			<< "<h1>Test page</h1>\n"
			<< "<p>Not Found file</p>\n";
		//<< "<h2>Request headers</h2>\n"
		//<< "<pre>" << buf << "</pre>\n"
		//<< "<em><small>Test C++ Http Server</small></em>\n";

		// Формируем весь ответ вместе с заголовками
		response << "HTTP/1.1 404 Not Found \r\n"
			<< "Content-Type: text/html; charset=utf-8\r\n"
			<< "Content-Length: " << response_body.str().length()
			<< "\r\n\r\n"
			<< response_body.str();
		send(clientInstance, response.str().c_str(), (int)response.str().length(), 0);
	}

	if (m_file)//if i openet the file i have to close it 
	{
		err = fclose(m_file);
		if (err == 0)
		{
			printf("The file was closed\n");
		}
		else
		{
			printf("The file was not closed\n");
		}
	}
}

void sendBodyToServer(string body, SOCKET clientInstance)
{
	//FILE* m_file;
	ofstream fout;

	time_t t;
	struct tm *t_m;
	t = time(NULL);
	t_m = localtime(&t);

	//err = fopen_s(&m_file, "test.txt", "w");
	fout.open("server.txt", ios_base::app);

	fout << body <<	"	Local time is: " << t_m->tm_hour << ":" << t_m->tm_min << ":" << t_m->tm_sec << endl;

	fout.close();

	cout << "Sending to test.txt is done." << endl;
}

static const string REGEX_GET = R"((GET)\s\/(.+)\s(HTTP.+))";
string getFilePath(string p_toParse)
{
	regex rx(REGEX_GET);
	string extractedSubmatchPath = { "" };
	//std::cout << REGEX_GET << ": " << std::regex_match(p_toParse, rx) << '\n';

	std::smatch pieces_match;
	if (std::regex_match(p_toParse, pieces_match, rx))
	{

		std::ssub_match sub_match = pieces_match[2];
		extractedSubmatchPath = sub_match.str();
		//std::cout << "  SUBMATCH " << 2 << ": " << extractedSubmatchPath << '\n';
		//std::cout << "regexes" << '\n';
		/*for (size_t i = 0; i < pieces_match.size(); ++i)
		{
		std::ssub_match sub_match = pieces_match[i];
		std::string piece = sub_match.str();
		std::cout << "  submatch " << i << ": " << piece << '\n';
		}*/
	}
	return extractedSubmatchPath;//if there is no match so the request is not HTTP it will return empty string
}

//support function to check what is in the buffer
void printBuffer(char* bufferPtr, int size)
{

	for (int i = 0; i < size; i++) {
		cout << *bufferPtr;
		bufferPtr++;
	}

}
void sendFile(FILE* m_file, SOCKET clientInstance)
{
	char statusLine[] = "HTTP/1.1 200 OK\r\n";
	char contentTypeLine[] = "Content-Type: text/html\r\n";

	fseek(m_file, 0, SEEK_END);
	int bufferSize = ftell(m_file);
	//cout << "The file lenght is :" << bufferSize << endl;;
	rewind(m_file);
	//this creates unique pointer to my array 
	unique_ptr<char[]> myBufferedFile = make_unique<char[]>(bufferSize);

	//this reads whole file into buffert.
	int numRead = fread_s(myBufferedFile.get(), bufferSize, sizeof(char), bufferSize, m_file);

	int totalSend = bufferSize + strlen(statusLine) + strlen(contentTypeLine);

	unique_ptr<char[]> myUniqueBufferToSend = make_unique<char[]>(totalSend);

	memcpy(myUniqueBufferToSend.get(), &statusLine, strlen(statusLine));
	memcpy(myUniqueBufferToSend.get() + strlen(statusLine), &contentTypeLine, strlen(contentTypeLine));
	memcpy(myUniqueBufferToSend.get() + strlen(statusLine) + strlen(contentTypeLine), myBufferedFile.get(), bufferSize);

	cout << "Sending response." << endl;

	int iResult = send(clientInstance, myUniqueBufferToSend.get(), totalSend, 0);
	if (iResult == SOCKET_ERROR)
	{
		wprintf(L"send failed with error: %d\n", WSAGetLastError());
		closesocket(clientInstance);
		WSACleanup();
	}

	cout << "Total bytes send: " << iResult << endl;

}
#endif
