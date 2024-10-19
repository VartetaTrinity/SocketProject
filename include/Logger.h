// Logger.cpp : Defines the functions for the static library.
//

#include <iostream>
#include <fstream>
using namespace std;

#define MAX_PATH_SIZE 256

// TODO: This is an example of a library function
//Singleton class
class CommonLogger
{
private:
	static CommonLogger* m_pLogger;
	static ofstream *oLogFile;
	CommonLogger();
public:
	static CommonLogger* GetLogger();

	void LogMessage(const char* sMsg);
	const char* GiveCurTimestamp();

	~CommonLogger();

};
