#define _CRT_SECURE_NO_WARNINGS //Suppress warnings on strcpy,ctime,itoa etc...
#include <iostream>
using namespace std;
// Don't forget to include "Ws2_32.lib" in the library list.
#include <fstream>
#include <winsock2.h>
#include <string.h>
#include <vector>
#include "utilityFunctions.h"
#pragma comment(lib, "Ws2_32.lib")
#define CRLF "\r\n"

#define DEBUG_OUTGOING_MESSAGE 0 //Use these to see content of messages sent
#define DEBUG_INCOMING_MESSAGE 0 //or received.
#define FAIL -1
#define TIMEOUT 0

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

enum eReqType {GET, HEAD, PUT, DELETE_REQ,OK=200,BAD_REQUEST=400,Forbidden=403,Not_Found=404,Request_Too_Large=413,Internal_Server_Error=500,NOT_IMPLEMENTED=501};

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
	string body;
};

struct SocketState
{
	SOCKET id;			// Socket handle
	int	recv;			// Receiving?
	int	send;			// Sending?
	int sendSubType;	// Sending sub-type
	char buffer[4096];  //4k
	string recvBuffer;
	string sendBuffer;
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
void readHeaders(char * & buffer,request & req);
void readBody(char * & buffer,request & req);
request makeNewReq();
int ParseHTTPMessage(char * buffer, request & reqinfo);
int makeresponse(request & reqinfo, string &sendbuffer);
void writeDateHeader(string & response);
void writeContentLengthHeader(string & response, int contentLength);
string ReqToString (eReqType methodType);
int actOnRequest(request & reqinfo);
void getFile(request & reqinfo);
void putFile(request & reqinfo);
void deleteFile(request & reqinfo);
bool operator==(const string& str,const char * str2);
void CleanURI(string & uri);
//Globals
struct SocketState sockets[MAX_SOCKETS]={0};
int socketsCount = 0;