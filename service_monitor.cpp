// service_monitor.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <Windows.h>

using namespace std;

SERVICE_STATUS ServiceStatus_power;
SERVICE_STATUS_HANDLE ServiceStatusHandle;
HANDLE hStopEvent = NULL;
HANDLE hPowerSettingEvent = NULL;
GUID PowerSettingsGuid = GUID_CONSOLE_DISPLAY_STATE;

VOID WINAPI ServiceMain(DWORD argc, LPTSTR* argv);
VOID WINAPI ServiceCtrlHandler(DWORD CtrlCode);
DWORD WINAPI ServiceWorkerThread(LPVOID lpParam);
SERVICE_STATUS ServiceStatus;


FILE* fptr;


VOID WINAPI ServiceMain(DWORD argc, LPTSTR* argv)
{
    DWORD Status = E_FAIL;
    HANDLE hThread = NULL;

    ServiceStatus_power.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    ServiceStatus_power.dwCurrentState = SERVICE_START_PENDING;
    ServiceStatus_power.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    ServiceStatus_power.dwWin32ExitCode = 0;
    ServiceStatus_power.dwServiceSpecificExitCode = 0;
    ServiceStatus_power.dwCheckPoint = 0;
    ServiceStatus_power.dwWaitHint = 0;

    ServiceStatusHandle = RegisterServiceCtrlHandler(TEXT("MyService"), ServiceCtrlHandler);

    if (ServiceStatusHandle == NULL)
    {
        cout << "Failed to register service control handler." << endl;
        goto EXIT;
    }

    ServiceStatus_power.dwCurrentState = SERVICE_RUNNING;
    ServiceStatus_power.dwCheckPoint = 0;
    ServiceStatus_power.dwWaitHint = 0;

    if (SetServiceStatus(ServiceStatusHandle, &ServiceStatus_power) == FALSE)
    {
        cout << "Failed to set service status to running." << endl;
    }

    hStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    if (hStopEvent == NULL)
    {
        cout << "Failed to create stop event." << endl;
        goto EXIT;
    }

    hPowerSettingEvent = RegisterPowerSettingNotification(hStopEvent, &PowerSettingsGuid, DEVICE_NOTIFY_SERVICE_HANDLE);

    if (hPowerSettingEvent == NULL)
    {
        cout << "Failed to register power setting notification." << endl;
        goto EXIT;
    }

    hThread = CreateThread(NULL, 0, ServiceWorkerThread, NULL, 0, NULL);

    if (hThread == NULL)
    {
        cout << "Failed to create worker thread." << endl;
        goto EXIT;
    }

    WaitForSingleObject(hStopEvent, INFINITE);

    ServiceStatus_power.dwCurrentState = SERVICE_STOP_PENDING;
    ServiceStatus_power.dwCheckPoint = 1;
    ServiceStatus_power.dwWaitHint = 10000;

    if (SetServiceStatus(ServiceStatusHandle, &ServiceStatus_power) == FALSE)
    {
        cout << "Failed to set service status to stop pending." << endl;
    }

    if (hThread != NULL)
    {
        WaitForSingleObject(hThread, INFINITE);
        CloseHandle(hThread);
    }

    if (hPowerSettingEvent != NULL)
    {
        UnregisterPowerSettingNotification(hPowerSettingEvent);
    }

    if (hStopEvent != NULL)
    {
        CloseHandle(hStopEvent);
    }

    ServiceStatus_power.dwCurrentState = SERVICE_STOPPED;
    ServiceStatus_power.dwCheckPoint = 0;
    ServiceStatus_power.dwWaitHint = 0;

    if (SetServiceStatus(ServiceStatusHandle, &ServiceStatus_power) == FALSE)
    {
        cout << "Failed to set service status to stopped." << endl;
    }

EXIT:
    return;
}

VOID WINAPI ServiceCtrlHandler(DWORD CtrlCode)
{
    switch (CtrlCode)
    {
    case SERVICE_CONTROL_STOP:
        ServiceStatus_power.dwCurrentState = SERVICE_STOP_PENDING;
        ServiceStatus_power.dwCheckPoint = 1;
        ServiceStatus_power.dwWaitHint = 10000;
        SetEvent(hStopEvent);
        break;

    default:
        break;
    }
}

DWORD WINAPI ServiceWorkerThread(LPVOID lpParam)
{
    DWORD dwResult = WaitForSingleObject(hStopEvent, 0);

    while (dwResult == WAIT_TIMEOUT)
    {
        dwResult = WaitForSingleObject(hStopEvent, INFINITE);

        if (dwResult == WAIT_OBJECT_0)
        {
            break;
        }

        if (dwResult == WAIT_FAILED)
        {
            cout << "Wait for stop event failed." << endl;
            break;
        }
    }

    return 0;
}

// Handler function to handle control signals
void WINAPI ServiceControlHandler(DWORD controlCode)
{
    SYSTEMTIME st;

    GetSystemTime(&st);
    fprintf(fptr, "%02d:%02d:%02d.%03d -%d\n", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, controlCode);

    switch (controlCode)
    {
    case SERVICE_CONTROL_STOP:
    case SERVICE_CONTROL_SHUTDOWN:

        ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
        // Perform cleanup tasks here
        SetServiceStatus(ServiceStatusHandle, &ServiceStatus);
        exit(0); // Exit gracefully
        break;

    default:
        break;
    }
}


int main(int argc, TCHAR* argv[])
{
    FILE* file_ptr; // declare a pointer to file
    const char* file_name = "c:\\tmp\\example.txt"; // replace with the name of your file
    const char* mode = "a"; // open file in read-only mode

    // try to open file
    errno_t error_code = fopen_s(&fptr, file_name, mode);
    SYSTEMTIME st;

    cout << error_code << endl;
    cout << "hellow." << endl;
    // Set the initial service status to running
    ServiceStatus.dwServiceType = SERVICE_WIN32;
    ServiceStatus.dwCurrentState = SERVICE_RUNNING;
    ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN | SERVICE_ACCEPT_PRESHUTDOWN;
    ServiceStatus.dwWin32ExitCode = 0;
    ServiceStatus.dwServiceSpecificExitCode = 0;
    ServiceStatus.dwCheckPoint = 0;
    ServiceStatus.dwWaitHint = 0;

    // Register the control handler function
    ServiceStatusHandle = RegisterServiceCtrlHandlerW(L"MyService", ServiceControlHandler);

    GetSystemTime(&st);
    fprintf(fptr, "%02d:%02d:%02d.%03d - Log entry\n", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
    // Check if the handler registration succeeded
    if (!ServiceStatusHandle)
    {
        return GetLastError();
    }

    // Set the service status to running
    if (!SetServiceStatus(ServiceStatusHandle, &ServiceStatus))
    {
        return GetLastError();
    }

    // Perform startup tasks here

    while (ServiceStatus.dwCurrentState == SERVICE_RUNNING)
    {
        // Perform the main service function here
        // ...
    }

    return 0;
}