#include "anti-debug.h"

bool CheckDebugger() {
    if (IsDebuggerAttached()) //check if debugger is attached
    {
        std::cout << "Debugger detected! Exiting." << std::endl;
        return true;
    }
    else if (CheckForRemoteDebugger()) //check if remote debugger is attached
    {
        std::cout << "Remote debugger detected! Exiting." << std::endl;
        return true;
    }
    else if (CheckForDebuggerProcesses()) //check if debugger processes are running
    {
        std::cout << "Debugger process detected! Exiting." << std::endl;
        return true;
    }
    else {
        std::cout << "No debugger detected." << std::endl;
        return false;
    }
}

//check if debugger is attached
bool IsDebuggerAttached() {
    return IsDebuggerPresent();
}

//check if debugger processes are running
bool CheckForDebuggerProcesses() {
    //list of common debugger processes
    const char* debuggerProcesses[] = {
        "ollydbg.exe",
        "x64dbg.exe",
        "windbg.exe",
        "gdb.exe",
        "ida.exe",
        "ida64.exe",
        "ImmunityDebugger.exe",
        "devenv.exe",
        "eclipse.exe",
        "radare2.exe",
        "studio.exe",
        "lldb.exe",
        "cheatengine.exe"
    };
    //number of debugger processes in the list
    int numDebuggerProcesses = sizeof(debuggerProcesses) / sizeof(debuggerProcesses[0]);

    //create a snapshot of all processes
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return false;
    }

    //iterate through all processes
    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);

    //check if the process is in the list of debugger processes
    if (Process32First(hSnapshot, &pe)) {
        do {
            //convert wide character string to regular string
            std::string exeFile = WideCharToString(pe.szExeFile);
            //compare the process name with the list of debugger processes
            for (int i = 0; i < numDebuggerProcesses; ++i) {
                if (_stricmp(exeFile.c_str(), debuggerProcesses[i]) == 0) {
                    //close the handle and return true if a debugger process was found
                    CloseHandle(hSnapshot);
                    return true;
                }
            }
        } while (Process32Next(hSnapshot, &pe));
    }

    //close the handle and return false if no debugger processes were found
    CloseHandle(hSnapshot);
    return false;
}

// Convert a wide character string to a regular string
std::string WideCharToString(const WCHAR* wideStr) {
    //get the size of the buffer needed to store the string
    int bufferSize = WideCharToMultiByte(CP_ACP, 0, wideStr, -1, NULL, 0, NULL, NULL);
    if (bufferSize == 0) {
        return "";
    }
    //allocate a buffer to store the string
    char* buffer = new char[bufferSize];
    //convert the wide character string to a regular string
    WideCharToMultiByte(CP_ACP, 0, wideStr, -1, buffer, bufferSize, NULL, NULL);
    //create a string from the buffer
    std::string str(buffer);
    //delete the buffer and return the string
    delete[] buffer;
    return str;
}

bool CheckForRemoteDebugger() {
    BOOL isDebuggerPresent = FALSE;
    if (CheckRemoteDebuggerPresent(GetCurrentProcess(), &isDebuggerPresent)) {
        return isDebuggerPresent;
    }
    else {
        // Si la fonction échoue, on suppose par défaut qu'il n'y a pas de débogueur distant
        return false;
    }
}