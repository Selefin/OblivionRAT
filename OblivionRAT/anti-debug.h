#include <Windows.h>
#include <TlHelp32.h>
#include <string>
#include <iostream>

bool BugCheck(); //check if debugger is attached
bool IsAttached(); //check if debugger is attached
bool CheckForProcesses(); //check if debugger processes are running
std::string WideCharToString(const WCHAR* wideStr); //convert a wide character string to a regular string
bool CheckForRemote(); //check if a remote debugger is attached