#define _CRT_SECURE_NO_WARNINGS //Suppress warnings on strcpy,ctime,itoa etc...
#include <iostream>
using namespace std;
// Don't forget to include "Ws2_32.lib" in the library list.
#include <winsock2.h>
#include <string.h>
#include <time.h>
#include <vector>
#include <winbase.h>
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

enum eReqType {GET, HEAD, PUT, DELETE_REQ,OK=200,BAD_REQUEST=400,Forbidden=403,Not_Found=404,Internal_Server_Error=500,NOT_IMPLEMENTED=501};

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
	char buffer[512];
	char sendBuffer[512];
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
request makeNewReq();
int Parse_HTTP_Header(char * buffer, request & reqinfo);
int makeResponce(request & reqinfo, char sendbuffer[]);
string ReqToString (eReqType methodType);
int actOnRequest(request & reqinfo);
void putFile(request & reqinfo);
void deleteFile(request & reqinfo);
bool exists(const char* filePath);
bool operator==(const string& str,const char * str2);
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
		request req=makeNewReq();
		if (sockets[index].len > 0)
		{
			Parse_HTTP_Header(sockets[index].buffer,req);
			sockets[index].send=SEND;
			makeResponce(req,sockets[index].sendBuffer);//EDIT_THIS?
		}
	}
}

void sendMessage(int index)
{
	int bytesSent = 0;
	SOCKET msgSocket = sockets[index].id;
	

	bytesSent = send(msgSocket, sockets[index].sendBuffer, (int)strlen(sockets[index].sendBuffer), 0);
	if (SOCKET_ERROR == bytesSent)
	{
		cout << "Web Server: Error at send(): " << WSAGetLastError() << endl;	
		return;
	}

	cout<<"Web Server: Sent: "<<bytesSent<<"\\"<<strlen(sockets[index].sendBuffer)<<" bytes of \"\n"<<sockets[index].sendBuffer<<"\n\" message.\n";	

	sockets[index].send = IDLE;
}

#define FAIL -1
/*  Parses a string and updates a request
information structure if necessary.    */

int Parse_HTTP_Header(char * buffer, request & reqinfo) {

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
	buffer++;
	for(int i=0; i<len-1; i++)
	{
		reqinfo.uri.push_back(buffer[0]);
		buffer++;//next char
	}
	if (buffer[0]!=' ')
	{
		reqinfo.methodType=BAD_REQUEST;
		return FAIL;
	}
	buffer++;//pass CRLF
	//Parse HTTP version
	if ( strncmp(buffer, "HTTP/1.0",8)==0 )
	{
		reqinfo.http_version_major=1;
		reqinfo.http_version_minor=0;
	}
	else if ( strncmp(buffer, "HTTP/1.1",8)==0 )
	{
		reqinfo.http_version_major=1;
		reqinfo.http_version_minor=1;
	}
	else
	{
		reqinfo.methodType=BAD_REQUEST;
		return FAIL;
	}
	buffer+=8;//jump past http/1.*
	readHeaders(buffer,reqinfo);
	if (reqinfo.methodType==BAD_REQUEST)
	{
		return FAIL;
	}
	return 0;
		



	
}

void passSpaces( char * & buff )
{
	while ( buff[0]!='\0' && isspace(buff[0]) )
		buff++;//read from next place
}

header makeHeader(const string & name, const string & Val )
{
	header head;
	head.name=name;
	head.val=Val;
	return head;
}
void readHeaders( char * & buffer,request & req )
{
	char * endptr;
	string temp1="";
	string temp2="";
	passSpaces(buffer);
	endptr = strchr(buffer, ':');
	if ( endptr == NULL ) //at least one header is a must(Host)
	{
		req.methodType = BAD_REQUEST;
	}
	while(endptr!=NULL)//as long as there are still headers to read
	{
		temp1="";
		temp2="";
		for (int i=0;i<endptr-buffer;i++)
		{
			temp1.push_back(buffer[i]);
		}
		buffer=endptr+1;//move past :
		passSpaces(buffer);
		endptr = strchr(buffer,'\n');//read until new line
		for (int i=0;i<endptr-buffer;i++)
		{
			temp2.push_back(buffer[i]);
		}
		buffer=endptr+1;
		//we only kepp the following headers, all others are ignored
		if (temp1=="Content-Length" || temp1=="Expect" ||temp1=="Host"||temp1=="User-Agent"|| temp1=="Connection")
		{
			req.headers.push_back(makeHeader(temp1, temp2));
		}
		



			if (buffer!=NULL)
			{
				endptr = strchr(buffer, ':');
			}
			else
			{
				return;
			}
			
	}

}

bool isLWS(char a)
{
	return (a=='\n'||a=='\t' || a==' ');
}
bool operator==(const string& str,const char * str2)
{
	const char * temp=str.c_str();
	return strcmp(temp,str2)==0;
}
request makeNewReq()
{
	request req;
	req.methodType=BAD_REQUEST;
	req.http_version_major=1;
	req.http_version_minor=1;
	req.uri="";
	return req;
}

int makeResponce( request & reqinfo, char sendbuffer[] )
{
	string responce="HTTP/";
	actOnRequest(reqinfo);//changes the req code to the send code(200,404,...)
	responce.push_back('0'+reqinfo.http_version_major);
	responce.push_back('.');
	responce.push_back(('0'+reqinfo.http_version_minor));
	responce+=ReqToString(reqinfo.methodType);
	responce+="\r\n\r\n";
	strcpy(sendbuffer,responce.c_str());
	
	return 1;



}
string ReqToString (eReqType methodType)
{
	switch(methodType)
	{
	case OK : return " 200 OK";
	case BAD_REQUEST : return " 400 Bad Request";
	case Forbidden : return " 403 Forbidden";
	case Not_Found : return " 404 Not Found";
	case NOT_IMPLEMENTED : return " 501 Not Implemented";
	default:return "500 Internal Server Error"; // if the method type is none of the above an error has occared
	}

}
int actOnRequest(request & reqinfo)
{
	switch(reqinfo.methodType)
	{
	case GET:
		//change this please
		reqinfo.methodType=NOT_IMPLEMENTED;
		break;
	case HEAD:
		reqinfo.methodType=NOT_IMPLEMENTED;
		break;
	case PUT:
		putFile(reqinfo);
		break;
	case DELETE_REQ:
		deleteFile(reqinfo);
		break;

	}
	return 1;
}


void putFile(request & reqinfo)
{

}
bool exists(const char* filePath)
{
	//This will get the file attributes bitlist of the file
	DWORD fileAtt = GetFileAttributesA(filePath);
	//If an error occurred it will equal to INVALID_FILE_ATTRIBUTES
	if(fileAtt == INVALID_FILE_ATTRIBUTES)
		return false;

	//If the path referers to a directory it should also not exists.
	return ( ( fileAtt & FILE_ATTRIBUTE_DIRECTORY ) == 0 ); 
}
bool isWriteProtected(const char* filePath)
{
	//This will get the file attributes bitlist of the file
	DWORD fileAtt = GetFileAttributesA(filePath);
	//If the path referers to a directory it should also not exists.
	return ( ( fileAtt & FILE_ATTRIBUTE_READONLY ) != 0 ); 
}

void deleteFile(request & req)
{

	if ( !exists(req.uri.c_str()) ) //file dosent exsist
	{
		req.methodType=Not_Found;

	}
	else
	{
		if (!isWriteProtected(req.uri.c_str()))
		{
			if( remove( req.uri.c_str() ) != 0 )
				req.methodType=Internal_Server_Error;
			else
				req.methodType=OK;
			
		}
		else
		{
			req.methodType=Forbidden;
		}
		
	}
		
}
