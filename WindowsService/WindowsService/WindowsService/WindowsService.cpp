#include <Windows.h>
#include <iostream>
#include <fstream>
#include <time.h>

//TODO - Need to make it modular for better maintenance and management of the code.
//Need to put the code for logging in a different library and call from any other module
//Need to put the code for socket in other header and source files and use it here to run as server

using namespace std;

#define MAX_TIME_LEN 26

SERVICE_STATUS        g_ServiceStatus = { 0 };
SERVICE_STATUS_HANDLE g_StatusHandle = NULL;
HANDLE                g_ServiceStopEvent = INVALID_HANDLE_VALUE;

VOID WINAPI __ServiceMain(DWORD argc, LPTSTR* argv);
VOID WINAPI __ServiceControlHandler(DWORD);
DWORD WINAPI __ServiceWorkerThread(LPVOID lpParam);

//Service logging - Instrumentation
ofstream oLogFile("C:/ServiceLog/WinSvcSocketServiceLog.txt", ios::app);
int err = 0;

const char* GiveCurTimestamp()
{
    time_t curTime;
    time(&curTime);
    char sTimeBuff[26] = { 0, };

    ctime_s(sTimeBuff, 26, &curTime);
    char* st = new char[30];
    strcpy_s(st, 26, sTimeBuff);
    return st;
}

int main()
{
    const char* sTime = GiveCurTimestamp();
    oLogFile << endl << sTime <<":"<<__LINE__<< ":The service main function called";
    delete[]sTime;

    SERVICE_TABLE_ENTRY ServiceTable[] =
    {
        {(char*)"RealTimeUpdateService", (LPSERVICE_MAIN_FUNCTION)__ServiceMain},
        {NULL, NULL}
    };

    if (StartServiceCtrlDispatcher(ServiceTable) == FALSE)
    {
        err = GetLastError();
        sTime = GiveCurTimestamp();
        oLogFile << endl<<sTime<<":"<< __LINE__ << ":The service dispatcher failed. Error:" << err;
        delete[]sTime;
        return err;
    }
    return 0;
}


VOID WINAPI __ServiceMain(DWORD argc, LPTSTR* argv)
{
    DWORD Status = E_FAIL;
    const char* sTime = NULL;
    // Register our service control handler with the SCM
    g_StatusHandle = RegisterServiceCtrlHandler("RealTimeUpdateService", __ServiceControlHandler);

    if (g_StatusHandle == NULL)
    {
        err = GetLastError();
        sTime = GiveCurTimestamp();
        oLogFile << endl <<sTime<< ":"<<__LINE__ << ":The RegisterServiceCtrlHandler failed. Returning..Error:" << err;
        delete[]sTime;
        return;
    }

    // Tell the service controller we are starting
    ZeroMemory(&g_ServiceStatus, sizeof(g_ServiceStatus));
    g_ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    g_ServiceStatus.dwControlsAccepted = 0;
    g_ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
    g_ServiceStatus.dwWin32ExitCode = 0;
    g_ServiceStatus.dwServiceSpecificExitCode = 0;
    g_ServiceStatus.dwCheckPoint = 0;

    if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
    {
        //Log error somewhere in log file
        err = GetLastError();
        sTime = GiveCurTimestamp();
        oLogFile << endl << sTime << ":"<< __LINE__<<":The SetServiceStatus failed to start the service. Returning..Error:" << err;
        delete[]sTime;
    }

    /*
     * Perform tasks necessary to start the service here
     */

     // Create a service stop event to wait on later
    g_ServiceStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (g_ServiceStopEvent == NULL)
    {
        // Error creating event
        // Tell service controller we are stopped and exit
        g_ServiceStatus.dwControlsAccepted = 0;
        g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
        g_ServiceStatus.dwWin32ExitCode = GetLastError();
        g_ServiceStatus.dwCheckPoint = 1;

        if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
        {
            //Log to some log file for tracking purposes
            err = GetLastError();
            sTime = GiveCurTimestamp();
            oLogFile << endl << sTime << ":" << __LINE__ << ":The SetServiceStatus failed to stop the service. Returning..Error:" << err;
            delete[]sTime;
        }
        return;
    }

    // Tell the service controller we are started
    g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    g_ServiceStatus.dwCurrentState = SERVICE_RUNNING;
    g_ServiceStatus.dwWin32ExitCode = 0;
    g_ServiceStatus.dwCheckPoint = 0;

    if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
    {
        //Log to some file for tracking purposes
        err = GetLastError();
        sTime = GiveCurTimestamp();
        oLogFile << endl << sTime << ":" << __LINE__ << ":The SetServiceStatus failed to update he service status to Running. Returning..Error:" << err;
        delete[]sTime;
    }

    // Start a thread that will perform the main task of the service
    HANDLE hThread = CreateThread(NULL, 0, __ServiceWorkerThread, NULL, 0, NULL);
    if (hThread == NULL)
    {
        err = GetLastError();
        sTime = GiveCurTimestamp();
        oLogFile << endl << sTime << ":" << __LINE__ << ":Unable to start the Worker Thread. Error:" << err;
        delete[]sTime;
        return;
    }

    // Wait until our worker thread exits signaling that the service needs to stop
    DWORD dw = WaitForSingleObject(hThread, INFINITE);
    if (dw == WAIT_ABANDONED)
    {
        err = GetLastError();
        sTime = GiveCurTimestamp();
        oLogFile << endl << sTime << ":" << __LINE__ << ":The WairForSingleObject failed.. Returning..Error:" << err;
        delete[]sTime;
        return;
    }


    /*
     * Perform any cleanup tasks
     */

    CloseHandle(g_ServiceStopEvent);

    // Tell the service controller we are stopped
    g_ServiceStatus.dwControlsAccepted = 0;
    g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
    g_ServiceStatus.dwWin32ExitCode = 0;
    g_ServiceStatus.dwCheckPoint = 3;

    if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
    {
        //Log to some file for debug purposes
        err = GetLastError();
        sTime = GiveCurTimestamp();
        oLogFile << endl << sTime << ":" << __LINE__ << ":The SetServiceStatus failed to stop the service. Returning..Error:" << err;
        delete[]sTime;
    }

    sTime = GiveCurTimestamp();
    oLogFile << endl << sTime << ":" << __LINE__ << "Service has been stopped";
    delete[]sTime;
    oLogFile.close();
}

VOID WINAPI __ServiceControlHandler(DWORD CtrlCode)
{
    const char* sTime = NULL;
    switch (CtrlCode)
    {
    case SERVICE_CONTROL_STOP:

        if (g_ServiceStatus.dwCurrentState != SERVICE_RUNNING)
            break;

        /*
         * Perform tasks necessary to stop the service here
         */

        g_ServiceStatus.dwControlsAccepted = 0;
        g_ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
        g_ServiceStatus.dwWin32ExitCode = 0;
        g_ServiceStatus.dwCheckPoint = 4;

        sTime = GiveCurTimestamp();
        oLogFile << endl << sTime << ":" << __LINE__ << ":Got the request to stop the service..";
        delete[]sTime;

        if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
        {
            //Log to some file for debug purposes 
            err = GetLastError();
            sTime = GiveCurTimestamp();
            oLogFile << endl << sTime << ":" << __LINE__ << ":The SetServiceStatus failed to stop_pending the service. Returning..Error:" << err;
            delete sTime;
        }

        // This will signal the worker thread to start shutting down
        SetEvent(g_ServiceStopEvent);

        break;

    default:
        break;
    }
}

DWORD WINAPI __ServiceWorkerThread(LPVOID lpParam)
{ 
    const char* sTime = NULL;
    //  Periodically check if the service has been requested to stop
    while (WaitForSingleObject(g_ServiceStopEvent, 0) != WAIT_OBJECT_0)
    {
        /*
         * Perform main service function here
         */
        sTime = GiveCurTimestamp();
        oLogFile << endl << sTime << ":" << __LINE__ << ":I'm Alive...";
        delete[]sTime;
        Sleep(2000);
    }
    
    sTime = GiveCurTimestamp();
    oLogFile << endl << sTime << ":" << __LINE__ << ":The worker thread is stopping now..";
    delete[]sTime;

    return ERROR_SUCCESS;
}