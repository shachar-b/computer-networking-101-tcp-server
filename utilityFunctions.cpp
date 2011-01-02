#include "utilityFunctions.h"

//************************************
// Method:    passSpaces - pass all LWS in the given string(by changeing the pointer!)
// FullName:  passSpaces
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: char *  & buff - a non null cstring
//************************************
void passSpaces( char * & buff )
{
	while (buff!="" && isLWS(buff[0]) )
		buff++;//read from next place
}

//************************************
// Method:    isLWS - returns true if and only if the char is a white space a tab a new line or carige return
// FullName:  isLWS
// Access:    public 
// Returns:   bool
// Qualifier:
// Parameter: char a
//************************************
bool isLWS(char a)
{
	return (a=='\n'||a=='\t' || a==' ' || a=='\r');
}

//************************************
// Method:    numOfDigits- returns the number of digits in the given (positve) number
// FullName:  numOfDigits
// Access:    public 
// Returns:   int
// Qualifier:
// Parameter: int num
//************************************
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

//************************************
// Method:    formatTime - a getter for the current time in printable format
// FullName:  formatTime
// Access:    public 
// Returns:   char*
// Qualifier:
//************************************
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

//************************************
// Method:    exists- revcives a file path and returns true iff the file/folder exist
// FullName:  exists
// Access:    public 
// Returns:   bool
// Qualifier:
// Parameter: const char * filePath- a non null path
//************************************
bool exists(const char* filePath)
{
	//This will get the file attributes bitlist of the file
	DWORD fileAtt = GetFileAttributesA(filePath);
	//If an error occurred it will equal to INVALID_FILE_ATTRIBUTES
	if(fileAtt == INVALID_FILE_ATTRIBUTES)
		return false;
	return true;// for now i allow folders//( ( fileAtt & FILE_ATTRIBUTE_DIRECTORY ) == 0 ); 
}

//************************************
// Method:    isWriteProtected- returns true iff the file is write protected
// FullName:  isWriteProtected
// Access:    public 
// Returns:   bool
// Qualifier:
// Parameter: const char * filePath - a non null path
//************************************
bool isWriteProtected(const char* filePath)
{
	//This will get the file attributes bitlist of the file
	DWORD fileAtt = GetFileAttributesA(filePath);
	//If the path referers to a directory it should also not exists.
	return ( ( fileAtt & FILE_ATTRIBUTE_READONLY ) != 0 ); 
}

//************************************
// Method:    makePath- makes a directory path
// FullName:  makePath
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: const string & path
//************************************
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

//************************************
// Method:    FolderExists-returns true iff the given folder path exsist
// FullName:  FolderExists
// Access:    public 
// Returns:   bool
// Qualifier:
// Parameter: char * strFolderName- a non null cstring
//************************************
bool FolderExists(char* strFolderName)
{
	return GetFileAttributesA(strFolderName) != INVALID_FILE_ATTRIBUTES;
}