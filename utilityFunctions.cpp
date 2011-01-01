#include "utilityFunctions.h"

void passSpaces( char * & buff )
{
	while (buff!="" && isLWS(buff[0]) )
		buff++;//read from next place
}

bool isLWS(char a)
{
	return (a=='\n'||a=='\t' || a==' ' || a=='\r');
}

int numOfDigits(int num)
{
	int result=1;
	while (num>9)
	{
		result++;
		num/=10;
	}
	return result;
}

char* formatTime()
{
	char* timeStr;
	// Get the current time.
	time_t timer;
	time(&timer);
	// Parse the current time to printable string.
	timeStr=_strdup(ctime(&timer));
	timeStr[strlen(timeStr)-1] = 0; //to remove the new-line from the created string
	return timeStr;
}

bool exists(const char* filePath)
{
	//This will get the file attributes bitlist of the file
	DWORD fileAtt = GetFileAttributesA(filePath);
	//If an error occurred it will equal to INVALID_FILE_ATTRIBUTES
	if(fileAtt == INVALID_FILE_ATTRIBUTES)
		return false;

	//If the path referers to a directory it should also not exists.
	return true;// for now i allow folders//( ( fileAtt & FILE_ATTRIBUTE_DIRECTORY ) == 0 ); 
}

bool isWriteProtected(const char* filePath)
{
	//This will get the file attributes bitlist of the file
	DWORD fileAtt = GetFileAttributesA(filePath);
	//If the path referers to a directory it should also not exists.
	return ( ( fileAtt & FILE_ATTRIBUTE_READONLY ) != 0 ); 
}

void makePath( const string &path )
{
	char* backSlash = "\\"; //Using char* for compatability with find_last_of
	char* directoryFullPath;
	int endOfDirectoryPath = path.find_last_of(backSlash);
	string systemCommand="mkdir ";
	if (endOfDirectoryPath!=-1)
	{
		directoryFullPath=new char[endOfDirectoryPath+1];
		strncpy(directoryFullPath,path.c_str(),endOfDirectoryPath);
		directoryFullPath[endOfDirectoryPath]='\0';
		if (!FolderExists(directoryFullPath))
		{
			systemCommand +=" ";
			systemCommand+=directoryFullPath;
			system(systemCommand.c_str());
		}
		delete []directoryFullPath;
	}
}

bool FolderExists(char* strFolderName)
{
	return GetFileAttributesA(strFolderName) != INVALID_FILE_ATTRIBUTES;
}