#define _CRT_SECURE_NO_WARNINGS //Suppress warnings on strcpy,ctime,itoa etc...
#include <iostream>
using namespace std;
#include <time.h>
#include <fstream>
#include <winsock2.h>

void passSpaces(char * & buff);
bool isLWS(char a);
int numOfDigits(int num);
bool exists(const char* filePath);
bool isWriteProtected(const char* filePath);
void makePath(const string &path);
bool FolderExists(char* strFolderName);
char* formatTime();