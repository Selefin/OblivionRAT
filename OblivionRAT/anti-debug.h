#include <Windows.h>
#include <TlHelp32.h>
#include <string>
#include <iostream>

bool CheckDebugger(); //check if debugger is attached
bool IsDebuggerAttached(); //check if debugger is attached
bool CheckForDebuggerProcesses(); //check if debugger processes are running
std::string WideCharToString(const WCHAR* wideStr); //convert a wide character string to a regular string
bool CheckForRemoteDebugger(); //check if a remote debugger is attached