#include "main.h"

// https://docs.microsoft.com/en-us/windows/win32/services/changing-a-service-configuration
int main(int argc, const char *argv[]) {
    const char *targetService = NULL;
    DWORD intervalMillis = 0;
    for (int i = 0; i < argc; ++i) {
        if (!_strcmpi(SERVICE_INDICATOR_ARG, argv[i])) {
            if (i + 1 >= argc) {
                break;
            }
            targetService = argv[i + 1];
            continue;
        }
        if (!_strcmpi(INTERVAL_INDICATOR_ARG, argv[i])) {
            if (i + 1 >= argc) {
                break;
            }
            const char* intervalString = argv[i + 1];
            size_t intervalStringLength = strlen(intervalString);
            if (intervalStringLength > 9) {
                intervalMillis = -1;
                continue;
            }
            BOOL allDigits = TRUE;
            for (size_t j = 0; j < intervalStringLength; ++j) {
                if (intervalString[j] < '0' || intervalString[j] > '9') {
                    allDigits = FALSE;
                    break;
                }
            }
            intervalMillis = allDigits ? atoi(argv[i + 1]) : -1;
            continue;
        }
    }

    BOOL firstTime = TRUE;
    DWORD returnCode = 0;
    while (TRUE) {
        if (firstTime) {
            firstTime = FALSE;
        } else {
            if (intervalMillis < 1) {
                return returnCode;
            }
            SleepEx(intervalMillis, FALSE);
        }

        if (NULL == targetService || intervalMillis == -1) {
            printf("WinSvc Continuum v%s\n", WSC_VERSION);
            printf("Usage: %s service_name [%s millis]\n", SERVICE_INDICATOR_ARG, INTERVAL_INDICATOR_ARG);
            returnCode = 1;
            continue;
        }

        SC_HANDLE schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
        if (NULL == schSCManager) {
            printf("OpenSCManager failed (%d)\n", GetLastError());
            returnCode = 1;
            continue;
        }

        SC_HANDLE schService = OpenService(schSCManager, targetService,
                                           SERVICE_QUERY_CONFIG | SERVICE_CHANGE_CONFIG | SERVICE_QUERY_STATUS |
                                           SERVICE_START);
        if (NULL == schService) {
            printf("OpenService failed (%d)\n", GetLastError());
            CloseServiceHandle(schSCManager);
            returnCode = 1;
            continue;
        }

        DWORD cbBytesNeeded = -1;

        QueryServiceConfigA(schService, NULL, 0, &cbBytesNeeded);
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
            printf("QueryServiceConfig failed (%d)\n", GetLastError());
            CloseServiceHandle(schService);
            CloseServiceHandle(schSCManager);
            returnCode = 1;
            continue;
        }

        QUERY_SERVICE_CONFIG *lpServiceConfig = (QUERY_SERVICE_CONFIG *) malloc(cbBytesNeeded);
        if (NULL == lpServiceConfig) {
            CloseServiceHandle(schService);
            CloseServiceHandle(schSCManager);
            returnCode = 3;
            continue;
        }
        if (!QueryServiceConfig(schService, lpServiceConfig, cbBytesNeeded, &cbBytesNeeded)) {
            printf("QueryServiceConfig failed (%d)\n", GetLastError());
            free(lpServiceConfig);
            CloseServiceHandle(schService);
            CloseServiceHandle(schSCManager);
            returnCode = 1;
            continue;
        }

        if (lpServiceConfig->dwStartType != SERVICE_AUTO_START) {
            printf("Service '%s' invalid start type: %d\n", targetService, lpServiceConfig->dwStartType);
            ChangeServiceConfig(schService, SERVICE_NO_CHANGE, SERVICE_AUTO_START, SERVICE_NO_CHANGE, NULL, NULL, NULL,
                                NULL, NULL, NULL, NULL);
            free(lpServiceConfig);
        }

        QueryServiceStatusEx(schService, SC_STATUS_PROCESS_INFO, NULL, 0, &cbBytesNeeded);
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
            printf("QueryServiceStatusEx failed (%d)\n", GetLastError());
            CloseServiceHandle(schService);
            CloseServiceHandle(schSCManager);
            returnCode = 1;
            continue;
        }

        SERVICE_STATUS_PROCESS *lpServiceStatus = (SERVICE_STATUS_PROCESS *) malloc(cbBytesNeeded);
        if (NULL == lpServiceStatus) {
            CloseServiceHandle(schService);
            CloseServiceHandle(schSCManager);
            returnCode = 3;
            continue;
        }
        if (!QueryServiceStatusEx(schService, SC_STATUS_PROCESS_INFO, (LPBYTE) lpServiceStatus, cbBytesNeeded,
                                  &cbBytesNeeded)) {
            printf("QueryServiceStatusEx failed (%d)\n", GetLastError());
            free(lpServiceStatus);
            CloseServiceHandle(schService);
            CloseServiceHandle(schSCManager);
            returnCode = 1;
            continue;
        }

        if (lpServiceStatus->dwCurrentState != SERVICE_RUNNING &&
            lpServiceStatus->dwCurrentState != SERVICE_START_PENDING) {
            printf("Service '%s' invalid state: %d\n", targetService, lpServiceStatus->dwCurrentState);
            switch (lpServiceStatus->dwCurrentState) {
                case SERVICE_PAUSED:
                case SERVICE_STOPPED:
                    if (!StartService(schService, 0, NULL)) {
                        printf("StartService failed (%d)\n", GetLastError());
                        returnCode = 1;
                        break;
                    }
                    printf("Service '%s' started\n", targetService);
                    break;
                default:
                    // one of transitional states, service will need to be dealt with later
                    returnCode = 2;
                    break;
            }
        }
        free(lpServiceStatus);

        CloseServiceHandle(schService);
        CloseServiceHandle(schSCManager);
    }
}
