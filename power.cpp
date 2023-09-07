#include <Windows.h>
#include <iostream>

using namespace std;

SERVICE_STATUS ServiceStatus_power;
SERVICE_STATUS_HANDLE ServiceStatusHandle;
HANDLE hStopEvent = NULL;
HANDLE hPowerSettingEvent = NULL;
GUID PowerSettingsGuid = GUID_CONSOLE_DISPLAY_STATE;

VOID WINAPI ServiceMain(DWORD argc, LPTSTR* argv);
VOID WINAPI ServiceCtrlHandler(DWORD CtrlCode);
DWORD WINAPI ServiceWorkerThread(LPVOID lpParam);

int main(int argc, char* argv[])
{
    SERVICE_TABLE_ENTRY ServiceTable[] =
    {
        { "MyService", (LPSERVICE_MAIN_FUNCTION)ServiceMain },
        { NULL, NULL }
    };

    if (StartServiceCtrlDispatcher(ServiceTable) == FALSE)
    {
        cout << "Failed to start service control dispatcher." << endl;
        return GetLastError();
    }

    return 0;
}

VOID WINAPI ServiceMain(DWORD argc, LPTSTR* argv)
{
    DWORD Status = E_FAIL;

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

    HANDLE hThread = CreateThread(NULL, 0, ServiceWorkerThread, NULL, 0, NULL);

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