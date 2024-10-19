// Logger.cpp : Defines the functions for the static library.
//

#include "pch.h"
#include "framework.h"
#include <Logger.h>
#include <Windows.h>
#include <iostream>
#include <fstream>
using namespace std;

#define MAX_PATH_SIZE 256

CommonLogger* CommonLogger::m_pLogger = NULL;
ofstream* CommonLogger::oLogFile = NULL;

CommonLogger::CommonLogger()
{

}

void CommonLogger::LogMessage(const char* sMsg)
{
	(*oLogFile) << endl << sMsg;
	(*oLogFile).flush();
}

CommonLogger* CommonLogger::GetLogger()
{
	if (m_pLogger == NULL)
	{
		m_pLogger = new CommonLogger();
		//Open the file for loogging
		//Read the environment variable LOGGER_FILE
		char sPathVal[255 + 1] = { 0, };
		GetEnvironmentVariable("LOGGER_FILE", sPathVal, MAX_PATH_SIZE);
		oLogFile = new ofstream(sPathVal, ios::app);
		return m_pLogger;
	}
}

CommonLogger::~CommonLogger()
{
	oLogFile->close();
	delete oLogFile;
	if (m_pLogger != NULL)
	{
		delete m_pLogger;
		m_pLogger = NULL;
	}
}

