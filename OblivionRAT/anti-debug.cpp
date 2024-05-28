#include "anti-debug.h"

bool BugCheck() {
    if (IsAttached()) //check if debugger is attached
    {
        std::cout << "\x44\x65\x62\x75\x67\x67\x65\x72\x20\x64\x65\x74\x65\x63\x74\x65\x64\x21\x20\x45\x78\x69\x74\x69\x6e\x67\x2e" << std::endl;
        return true;
    }
    else if (CheckForRemote()) //check if remote debugger is attached
    {
        std::cout << "\x52\x65\x6d\x6f\x74\x65\x20\x64\x65\x62\x75\x67\x67\x65\x72\x20\x64\x65\x74\x65\x63\x74\x65\x64\x21\x20\x45\x78\x69\x74\x69\x6e\x67\x2e" << std::endl;
        return true;
    }
    else if (CheckForProcesses()) //check if debugger processes are running
    {
        std::cout << "\x44\x65\x62\x75\x67\x67\x65\x72\x20\x70\x72\x6f\x63\x65\x73\x73\x20\x64\x65\x74\x65\x63\x74\x65\x64\x21\x20\x45\x78\x69\x74\x69\x6e\x67\x2e" << std::endl;
        return true;
    }
    else {
        std::cout << "\x4e\x6f\x20\x64\x65\x62\x75\x67\x67\x65\x72\x20\x64\x65\x74\x65\x63\x74\x65\x64\x2e" << std::endl;
        return false;
    }
}

//check if debugger is attached
bool IsAttached() {
    return IsDebuggerPresent();
}

//check if debugger processes are running
bool CheckForProcesses() {
    //list of common debugger processes
    const char* processes[] = {
        "\x6f\x6c\x6c\x79\x64\x62\x67\x2e\x65\x78\x65",
        "\x78\x36\x34\x64\x62\x67\x2e\x65\x78\x65",
        "\x77\x69\x6e\x64\x62\x67\x2e\x65\x78\x65",
        "\x67\x64\x62\x2e\x65\x78\x65",
        "\x69\x64\x61\x2e\x65\x78\x65",
        "\x69\x64\x61\x36\x34\x2e\x65\x78\x65",
        "\x49\x6d\x6d\x75\x6e\x69\x74\x79\x44\x65\x62\x75\x67\x67\x65\x72\x2e\x65\x78\x65",
        "\x64\x65\x76\x65\x6e\x76\x2e\x65\x78\x65",
        "\x65\x63\x6c\x69\x70\x73\x65\x2e\x65\x78\x65",
        "\x72\x61\x64\x61\x72\x65\x32\x2e\x65\x78\x65",
        "\x73\x74\x75\x64\x69\x6f\x2e\x65\x78\x65",
        "\x6c\x6c\x64\x62\x2e\x65\x78\x65",
        "\x63\x68\x65\x61\x74\x65\x6e\x67\x69\x6e\x65\x2e\x65\x78\x65"
    };
    //number of debugger processes in the list
    int num = sizeof(processes) / sizeof(processes[0]);

    //create a snapshot of all processes
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) {
        return false;
    }

    //iterate through all processes
    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);

    //check if the process is in the list of debugger processes
    if (Process32First(snap, &pe)) {
        do {
            //convert wide character string to regular string
            std::string exeFile = WideCharToString(pe.szExeFile);
            //compare the process name with the list of debugger processes
            for (int i = 0; i < num; ++i) {
                if (_stricmp(exeFile.c_str(), processes[i]) == 0) {
                    //close the handle and return true if a debugger process was found
                    CloseHandle(snap);
                    if (true) {
                        return true;
					}
                    else
                    {
                        return false;
                    }
                }
            }
        } while (Process32Next(snap, &pe));
    }

    //close the handle and return false if no debugger processes were found
    CloseHandle(snap);
    if (false) {
		return true;
	}
    else
    {
        return false;
    }
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

bool CheckForRemote() {
    BOOL isDebuggerPresent = FALSE;
    if (CheckRemoteDebuggerPresent(GetCurrentProcess(), &isDebuggerPresent)) {
        return isDebuggerPresent;
    }
    else {
        // Si la fonction échoue, on suppose par défaut qu'il n'y a pas de débogueur distant
        return false;
    }
}