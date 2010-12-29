#define _CRT_SECURE_NO_WARNINGS //Suppress warnings on strcpy,ctime,itoa etc...
#include <iostream>
using namespace std;
// Don't forget to include "Ws2_32.lib" in the library list.
#include <winsock2.h>
#include <string.h>
#include <time.h>
#include <vector>
#pragma comment(lib, "Ws2_32.lib")
#define CRLF "\r\n"

//Consts
const int LISTEN_PORT = 80;
const int MAX_SOCKETS = 60;
const int EMPTY = 0;
const int LISTEN  = 1;
const int RECEIVE = 2;
const int IDLE = 3;
const int SEND = 4;
//Services
const int SEND_TIME = 1;
const int SEND_SECONDS = 2;

enum eReqType {GET, HEAD, PUT, DELETE_REQ,BAD_REQUEST=400,NOT_IMPLEMENTED=501};

//Structs
struct header{
	string name;
	string val;
};

struct request
{
	eReqType methodType;
	string uri;//path
	int http_version_major;
	int http_version_minor;
	vector<header> headers;
};

struct SocketState
{
	SOCKET id;			// Socket handle
	int	recv;			// Receiving?
	int	send;			// Sending?
	int sendSubType;	// Sending sub-type
	char buffer[128];
	int len;
};

//Function declarations
void initWinsock();
SOCKET setupSocket();
bool closeWinsock(SOCKET listenSocket);
bool addSocket(SOCKET id, int what);
void removeSocket(int index);
void acceptConnection(int index);
void receiveMessage(int index);
void sendMessage(int index);
void passSpaces(char * & buff);
void readHeaders(char * & buffer,request & req);
bool isLWS(char a);

//Globals
struct SocketState sockets[MAX_SOCKETS]={0};
int socketsCount = 0;


//Main
//------------------------------------------------------------------------------//
void main() 
{
	initWinsock();
	SOCKET listenSocket = setupSocket();
	if (listenSocket==ERROR)
		return; //Upon discovering an error - quit the application.


    // Listen on the Socket for incoming connections.
	// The listen system call determines the length of the queue of incoming connection
	// requests for a socket by setting the backlog parameter.

    if (SOCKET_ERROR == listen(listenSocket, 5))
	{
		cout << "Web Server: Error at listen(): " << WSAGetLastError() << endl;
        closesocket(listenSocket);
		WSACleanup();
        return;
	}

	addSocket(listenSocket, LISTEN);

    // Accept connections and handle them one by one.
    // The accept() system call returns another socket descriptor while
    // continuing to listen on the listen socket. 

	while (true)
	{
		// The select function determines the status of one or more sockets,
		// waiting if necessary, to perform asynchronous I/O. Use fd_sets for
		// sets of handles for reading, writing and exceptions. select gets "timeout" for waiting
		// and still performing other operations (Use NULL for blocking). Finally,
		// select returns the number of descriptors which are ready for use (use FD_ISSET
		// macro to check which descriptor in each set is ready to be used).
		fd_set waitRecv;

		FD_ZERO(&waitRecv);

		for (int i = 0; i < MAX_SOCKETS; i++)
		{
			if ((sockets[i].recv == LISTEN) || (sockets[i].recv == RECEIVE))
				FD_SET(sockets[i].id, &waitRecv);
		}


		fd_set waitSend;
		FD_ZERO(&waitSend);
		for (int i = 0; i < MAX_SOCKETS; i++)
		{
			if (sockets[i].send == SEND)
				FD_SET(sockets[i].id, &waitSend);
		}


		//
		// Wait for interesting event.
		// Note: First argument is ignored. The fourth is for exceptions.
		// And as written above the last is a timeout, hence we are blocked if nothing happens.

		int nfd;
		nfd = select(0, &waitRecv, &waitSend, NULL, NULL);
		if (nfd == SOCKET_ERROR)
		{
			cout <<"Web Server: Error at select(): " << WSAGetLastError() << endl;
			WSACleanup();
			return;
		}


		for (int i = 0; i < MAX_SOCKETS && nfd > 0; i++)
		{
			if (FD_ISSET(sockets[i].id, &waitRecv))
			{
				nfd--;
				switch (sockets[i].recv)
				{
				case LISTEN:
					acceptConnection(i);
					break;
				case RECEIVE:
					receiveMessage(i);
					break;
				}
			}
		}


		for (int i = 0; i < MAX_SOCKETS && nfd > 0; i++)
		{
			if (FD_ISSET(sockets[i].id, &waitSend))
			{
				nfd--;
				switch (sockets[i].send)
				{
				case SEND:
					sendMessage(i);
					break;
				}
			}
		}
	}
	closeWinsock(listenSocket);
}

//Function implementations:
//------------------------------------------------------------------------------------//
void initWinsock()
{
	// Initialize Winsock (Windows Sockets).
	// Create a WSADATA object called wsaData.
	// The WSADATA structure contains information about the Windows 
	// Sockets implementation.

	WSAData wsaData; 

	// Call WSAStartup and return its value as an integer and check for errors.
	// The WSAStartup function initiates the use of WS2_32.DLL by a process.
	// First parameter is the version number 2.2.
	// The WSACleanup function destructs the use of WS2_32.DLL by a process.

	if (NO_ERROR != WSAStartup(MAKEWORD(2,2), &wsaData))
	{
		cout<<"Web Server: Error at WSAStartup()\n";
	}
}

SOCKET setupSocket()
{
	// Server side:
	// Create and bind a socket to an internet address.
	// Listen through the socket for incoming connections.
	// After initialization, a SOCKET object is ready to be instantiated.
	// Create a SOCKET object called listenSocket. 
	// For this application:	use the Internet address family (AF_INET), 

	//							streaming sockets (SOCK_STREAM), 

	//							and the TCP/IP protocol (IPPROTO_TCP).

	SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);


	// Check for errors to ensure that the socket is a valid socket.
	// Error detection is a key part of successful networking code. 
	// If the socket call fails, it returns INVALID_SOCKET. 
	// The if statement in the previous code is used to catch any errors that
	// may have occurred while creating the socket. WSAGetLastError returns an 
	// error number associated with the last error that occurred.

	if (INVALID_SOCKET == listenSocket)
	{
		cout<<"Web Server: Error at socket(): "<<WSAGetLastError()<<endl;
		WSACleanup();
		return ERROR;
	}
	// For a server to communicate on a network, it must bind the socket to 
	// a network address.
	// Need to assemble the required data for connection in sockaddr structure.
	// Create a sockaddr_in object called serverService. 

	sockaddr_in serverService;

	// Address family (must be AF_INET - Internet address family).

	serverService.sin_family = AF_INET; 

	// IP address. The sin_addr is a union (s_addr is a unsigned long 
	// (4 bytes) data type).
	// inet_addr (Internet address) is used to convert a string (char *) 
	// into unsigned long.
	// The IP address is INADDR_ANY to accept connections on all interfaces.

	serverService.sin_addr.s_addr = INADDR_ANY;
	// IP Port. The htons (host to network - short) function converts an
	// unsigned short from host to TCP/IP network byte order 
	// (which is big-endian).

	serverService.sin_port = htons(LISTEN_PORT);


	// Bind the socket for client's requests.
	// The bind function specifies which port the server is listenning on.
	// The function uses the socket handler, the sockaddr structure (which
	// defines properties of the desired connection) and the length of the
	// sockaddr structure (in bytes).

	if (SOCKET_ERROR == bind(listenSocket, (SOCKADDR *) &serverService, sizeof(serverService))) 
	{
		cout<<"Web Server: Error at bind(): "<<WSAGetLastError()<<endl;
		closesocket(listenSocket);
		WSACleanup();
		return ERROR;
	}

	return listenSocket;
}

bool closeWinsock(SOCKET listenSocket){
	// Closing connections and Winsock.
	cout<<"Web Server: Closing Connection.\n";
	closesocket(listenSocket);
	WSACleanup();
	return false;
}

bool addSocket(SOCKET id, int what)
{
	for (int i = 0; i < MAX_SOCKETS; i++)
	{
		if (sockets[i].recv == EMPTY)
		{
			sockets[i].id = id;
			sockets[i].recv = what;
			sockets[i].send = IDLE;
			sockets[i].len = 0;
			socketsCount++;
			return (true);
		}
	}
	return (false);
}

void removeSocket(int index)
{
	sockets[index].recv = EMPTY;
	sockets[index].send = EMPTY;
	socketsCount--;
}

void acceptConnection(int index)
{
	SOCKET id = sockets[index].id;
	struct sockaddr_in from;		// Address of sending partner
	int fromLen = sizeof(from);
	SOCKET msgSocket = accept(id, (struct sockaddr *)&from, &fromLen);

	if (INVALID_SOCKET == msgSocket)
	{ 
		 cout << "Web Server: Error at accept(): " << WSAGetLastError() << endl; 		 
		 return;
	}

	cout << "Web Server: Client "<<inet_ntoa(from.sin_addr)<<":"<<ntohs(from.sin_port)<<" is connected." << endl;
	// Set the socket to be in non-blocking mode.

	unsigned long flag=1;
	if (ioctlsocket(msgSocket, FIONBIO, &flag) != 0)
	{
		cout<<"Web Server: Error at ioctlsocket(): "<<WSAGetLastError()<<endl;
	}

	if (addSocket(msgSocket, RECEIVE) == false)
	{
		cout<<"\t\tToo many connections, dropped!\n";
		closesocket(id);
	}
	return;
}

void receiveMessage(int index)
{
	SOCKET msgSocket = sockets[index].id;
	int len = sockets[index].len;
	int bytesRecv = recv(msgSocket, &sockets[index].buffer[len], sizeof(sockets[index].buffer) - len, 0);
	if (SOCKET_ERROR == bytesRecv)
	{
		cout << "Web Server: Error at recv(): " << WSAGetLastError() << endl;
		closesocket(msgSocket);			
		removeSocket(index);
		return;
	}

	if (bytesRecv == 0)
	{
		closesocket(msgSocket);			
		removeSocket(index);
		return;
	}

	else
	{
		sockets[index].buffer[len + bytesRecv] = '\0'; //add the null-terminating to make it a string
		cout<<"Web Server: Received: "<<bytesRecv<<" bytes of \n\"\n"<<&sockets[index].buffer[len]<<"\" message.\n";
		sockets[index].len += bytesRecv;

		if (sockets[index].len > 0)
		{
			if (strncmp(sockets[index].buffer, "TimeString", 10) == 0)
			{
				sockets[index].send  = SEND;
				sockets[index].sendSubType = SEND_TIME;
				memcpy(sockets[index].buffer, &sockets[index].buffer[10], sockets[index].len - 10);
				sockets[index].len -= 10;
				return;
			}
			else if (strncmp(sockets[index].buffer, "SecondsSince1970", 16) == 0)
			{
				sockets[index].send  = SEND;
				sockets[index].sendSubType = SEND_SECONDS;
				memcpy(sockets[index].buffer, &sockets[index].buffer[16], sockets[index].len - 16);
				sockets[index].len -= 16;
				return;
			}
			else if (strncmp(sockets[index].buffer, "Exit", 4) == 0)
			{
				closesocket(msgSocket);
				removeSocket(index);
				return;
			}
			else
			{
				sockets[index].send  = SEND;
				sockets[index].sendSubType = NOT_IMPLEMENTED;
				memcpy(sockets[index].buffer, &sockets[index].buffer[38], sockets[index].len - 38);
				sockets[index].len -= 38;
				return;
			}
		}
	}
}

void sendMessage(int index)
{
	int bytesSent = 0;
	char sendBuff[255];

	SOCKET msgSocket = sockets[index].id;
	if (sockets[index].sendSubType == SEND_TIME)
	{
		// Answer client's request by the current time string.
		// Get the current time.
		time_t timer;
		time(&timer);
		// Parse the current time to printable string.
		strcpy(sendBuff, ctime(&timer));
		sendBuff[strlen(sendBuff)-1] = 0; //to remove the new-line from the created string
	}
	else if(sockets[index].sendSubType == SEND_SECONDS)
	{
		// Answer client's request by the current time in seconds.
		// Get the current time.
		time_t timer;
		time(&timer);
		// Convert the number to string.
		_itoa((int)timer, sendBuff, 10);		
	}
	else if (sockets[index].sendSubType == NOT_IMPLEMENTED)
	{
		strcpy(sendBuff,"HTTP/1.1 501 NOT IMPLEMENTED\r\n\r\n");
	}

	bytesSent = send(msgSocket, sendBuff, (int)strlen(sendBuff), 0);
	if (SOCKET_ERROR == bytesSent)
	{
		cout << "Web Server: Error at send(): " << WSAGetLastError() << endl;	
		return;
	}

	cout<<"Web Server: Sent: "<<bytesSent<<"\\"<<strlen(sendBuff)<<" bytes of \"\n"<<sendBuff<<"\n\" message.\n";	

	sockets[index].send = IDLE;
}

#define FAIL -1
/*  Parses a string and updates a request
information structure if necessary.    */

int Parse_HTTP_Header(char * buffer, request & reqinfo) {

	int first_header = 1;
	char      *temp;
	char      *endptr;
	int        len;


	/*  If first_header is 0, this is the first line of
	the HTTP request, so this should be the request line.  */


	/*  Get the request method, which is case-sensitive. This
	version of the server only supports the GET, HEAD, PUT, DELETE
	request methods.                                        */

	if ( strncmp(buffer, "GET ", 4)==0 ) {
		reqinfo.methodType = GET;
		buffer += 4;
	}
	else if ( strncmp(buffer, "HEAD ", 5)==0 ) {
		reqinfo.methodType = HEAD;
		buffer += 5;
	}
	else if ( strncmp(buffer, "PUT ", 4)==0 ) {
		reqinfo.methodType = PUT;
		buffer += 4;
	}
	else if ( strncmp(buffer, "DELETE ", 7)==0 ) {
		reqinfo.methodType = DELETE_REQ;
		buffer += 7;
	}
	else {
		reqinfo.methodType = NOT_IMPLEMENTED;
		return FAIL;
	}
	/*  Skip to start of resource  */

	passSpaces(buffer);


	/*  Calculate string length of resource...  */

	endptr = strchr(buffer, ' ');//find next white space
	if ( endptr == NULL )
		len = strlen(buffer);
	else
		len = (int)(endptr - buffer);
	if ( len == 0 ) {
		reqinfo.methodType=BAD_REQUEST;
		return FAIL;
	}

	/*  ...and store it in the request information structure.  */
	reqinfo.uri="";//init
	for(int i=0; i<len; i++)
	{
		reqinfo.uri.push_back(buffer[0]);
		buffer++;//next char
	}
	if (buffer[0]!='\n')
	{
		reqinfo.methodType=BAD_REQUEST;
		return FAIL;
	}
	buffer++;//pass CRLF
	//Parse HTTP version
	if ( strncmp(buffer, "HTTP/1.0",9) )
	{
		reqinfo.http_version_major=1;
		reqinfo.http_version_minor=0;
	}
	else if ( strncmp(buffer, "HTTP/1.1",9) )
	{
		reqinfo.http_version_major=1;
		reqinfo.http_version_minor=1;
	}
	else
	{
		reqinfo.methodType=BAD_REQUEST;
		return FAIL;
	}

	readHeaders(buffer,reqinfo);
	
	endptr = strchr(buffer, ':');
	if ( endptr == NULL ) {
		reqinfo.methodType = BAD_REQUEST;
		return FAIL;
	}
	
	temp = new char[(endptr - buffer) + 1];
	strncpy(temp, buffer, (endptr - buffer));
	temp[endptr - buffer]='\0';
	_strupr(temp);
	


	/*  Increment buffer so that it now points to the value.
	If there is no value, just return.                    */

	buffer = endptr + 1;
	while ( *buffer && isspace(*buffer) )
		++buffer;
	if ( *buffer == '\0' )
		return 0;


	/*  Now update the request information structure with the
	appropriate field value. This version only supports the
	"Referer:" and "User-Agent:" headers, ignoring all others.  */

	if ( !strcmp(temp, "USER-AGENT") ) {
		reqinfo->useragent = malloc( strlen(buffer) + 1 );
		strcpy(reqinfo->useragent, buffer);
	}
	else if ( !strcmp(temp, "REFERER") ) {
		reqinfo->referer = malloc( strlen(buffer) + 1 );
		strcpy(reqinfo->referer, buffer);
	}

	free(temp);
	return 0;
}

void passSpaces( char * & buff )
{
	while ( buff[0]!='\0' && isspace(buff[0]) )
		buff++;//read from next place
}

void readHeaders( char * & buffer,request & req )
{
	char * endptr;
	string temp1="";
	string temp2="";
	endptr = strchr(buffer, ':');
	while(endptr!=NULL)//as long as there are still headers to read
	{
		temp1="";
		temp2="";
		if (endptr==NULL)
		{
			return;//no more
		}
		for (int i=0;i<endptr-buffer;i++)
		{
			temp1.push_back(buffer[0]);
			buffer++;
		}
		buffer++;//move past :
		endptr = strchr(buffer, '\n');//read until new line
		for (int i=0;i<endptr-buffer;i++)
		{
			temp2.push_back(buffer[0]);
			buffer++;
		}
		req.headers


			endptr = strchr(buffer, ':');
	}

}

bool isLWS(char a)
{
	return (a=='\n'||a=='\t' || a==' ');
}