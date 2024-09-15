#include <Windows.h>
#include <iostream>
#include <fstream>
using namespace std;

SERVICE_STATUS        g_ServiceStatus = { 0 };
SERVICE_STATUS_HANDLE g_StatusHandle = NULL;
HANDLE                g_ServiceStopEvent = INVALID_HANDLE_VALUE;

VOID WINAPI __ServiceMain(DWORD argc, LPTSTR* argv);
VOID WINAPI __ServiceControlHandler(DWORD);
DWORD WINAPI __ServiceWorkerThread(LPVOID lpParam);

//Service logging - Instrumentation
ofstream oLogFile("C:/ServiceLog/WinSvcSocketServiceLog.txt", ios::app);
int err = 0;

int main()
{
    oLogFile << endl <<__LINE__<< ":The service main function called";
    

    SERVICE_TABLE_ENTRY ServiceTable[] =
    {
        {(char*)"RealTimeUpdateService", (LPSERVICE_MAIN_FUNCTION)__ServiceMain},
        {NULL, NULL}
    };

    if (StartServiceCtrlDispatcher(ServiceTable) == FALSE)
    {
        err = GetLastError();
        oLogFile << endl<< __LINE__<< ":The service dispatcher failed. Error:" << err;
        return err;
    }
    return 0;
}


VOID WINAPI __ServiceMain(DWORD argc, LPTSTR* argv)
{
    DWORD Status = E_FAIL;

    // Register our service control handler with the SCM
    g_StatusHandle = RegisterServiceCtrlHandler("RealTimeUpdateService", __ServiceControlHandler);

    if (g_StatusHandle == NULL)
    {
        err = GetLastError();
        oLogFile << endl <<__LINE__<< ":The RegisterServiceCtrlHandler failed. Returning..Error:" << err;
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
        oLogFile << endl <<__LINE__<< ":The SetServiceStatus failed to start the service. Returning..Error:" << err;
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
            oLogFile << endl << __LINE__ << ":The SetServiceStatus failed to stop the service. Returning..Error:" << err;
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
        oLogFile << endl << __LINE__ << ":The SetServiceStatus failed to update he service status to Running. Returning..Error:" << err;
    }

    // Start a thread that will perform the main task of the service
    HANDLE hThread = CreateThread(NULL, 0, __ServiceWorkerThread, NULL, 0, NULL);
    if (hThread == NULL)
    {
        err = GetLastError();
        oLogFile << endl << __LINE__ << ":Unable to start the Worker Thread. Error:" << err;
        return;
    }

    // Wait until our worker thread exits signaling that the service needs to stop
    DWORD dw = WaitForSingleObject(hThread, INFINITE);
    if (dw == WAIT_ABANDONED)
    {
        err = GetLastError();
        oLogFile << endl << __LINE__ << ":The WairForSingleObject failed.. Returning..Error:" << err;
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
        oLogFile << endl << __LINE__ << ":The SetServiceStatus failed to stop the service. Returning..Error:" << err;
    }

    oLogFile << endl << "Service has been stopped";
    oLogFile.close();
}

VOID WINAPI __ServiceControlHandler(DWORD CtrlCode)
{
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

        oLogFile << endl << __LINE__ << ":Got the request to stop the service..";

        if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
        {
            //Log to some file for debug purposes 
            err = GetLastError();
            oLogFile << endl << __LINE__ << ":The SetServiceStatus failed to stop_pending the service. Returning..Error:" << err;
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
    //  Periodically check if the service has been requested to stop
    while (WaitForSingleObject(g_ServiceStopEvent, 0) != WAIT_OBJECT_0)
    {
        /*
         * Perform main service function here
         */
        oLogFile << endl << __LINE__ << ":I'm Alive...";
        Sleep(2000);
    }
    
    oLogFile << endl << __LINE__ << ":The worker thread is stopping now..";

    return ERROR_SUCCESS;
}