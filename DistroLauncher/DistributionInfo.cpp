//
//    Copyright (C) Microsoft.  All rights reserved.
// Licensed under the terms described in the LICENSE file in the root of this project.
//

#include "stdafx.h"

bool DistributionInfo::CreateUser(std::wstring_view userName)
{
    // Create the user account.
    DWORD exitCode;
    std::wstring commandLine = L"/usr/bin/useradd -m ";
    commandLine += userName;
    HRESULT hr = g_wslApi.WslLaunchInteractive(commandLine.c_str(), true, &exitCode);
    if ((FAILED(hr)) || (exitCode != 0))
    {
        return false;
    }

    commandLine = L"/usr/bin/passwd ";
    commandLine += userName;
    hr = g_wslApi.WslLaunchInteractive(commandLine.c_str(), true, &exitCode);
    if ((FAILED(hr)) || (exitCode != 0))
    {
        // Delete the user if the group add command failed.
        commandLine = L"/usr/bin/userdel ";
        commandLine += userName;
        g_wslApi.WslLaunchInteractive(commandLine.c_str(), true, &exitCode);
        return false;
    }
    
    commandLine = L"/usr/bin/sed -i 's/# %wheel ALL=(ALL) ALL/%wheel ALL=(ALL) ALL/g' /etc/sudoers";
    hr = g_wslApi.WslLaunchInteractive(commandLine.c_str(), true, &exitCode);
    if ((FAILED(hr)) || (exitCode != 0))
    {
        // Delete the user if the group add command failed.
        commandLine = L"/usr/bin/userdel ";
        commandLine += userName;
        g_wslApi.WslLaunchInteractive(commandLine.c_str(), true, &exitCode);
        return false;
    }

    // Add the user account to any relevant groups.
    commandLine = L"/usr/bin/usermod -aG wheel ";
    commandLine += userName;
    hr = g_wslApi.WslLaunchInteractive(commandLine.c_str(), true, &exitCode);
    if ((FAILED(hr)) || (exitCode != 0))
    {
        // Delete the user if the group add command failed.
        commandLine = L"/usr/bin/userdel ";
        commandLine += userName;
        g_wslApi.WslLaunchInteractive(commandLine.c_str(), true, &exitCode);
        return false;
    }

    return true;
}

bool DistributionInfo::ConfigurePacman()
{
    // Update packages and allow all required PGP keys
    DWORD exitCode;
    std::wstring commandLine = L"/usr/bin/pacman -Syu --noconfirm";
    HRESULT hr = g_wslApi.WslLaunchInteractive(commandLine.c_str(), true, &exitCode);
    if ((FAILED(hr)) || (exitCode != 0))
    {
        return false;
    }

    // Initialize keyring for pacman
    commandLine = L"/usr/bin/pacman-key --init";
    hr = g_wslApi.WslLaunchInteractive(commandLine.c_str(), true, &exitCode);
    if ((FAILED(hr)) || (exitCode != 0))
    {
        return false;
    }

    // Reload the default keys
    commandLine = L"/usr/bin/pacman-key --populate";
    hr = g_wslApi.WslLaunchInteractive(commandLine.c_str(), true, &exitCode);
    if ((FAILED(hr)) || (exitCode != 0))
    {
        return false;
    }

    return true;
}

ULONG DistributionInfo::QueryUid(std::wstring_view userName)
{
    // Create a pipe to read the output of the launched process.
    HANDLE readPipe;
    HANDLE writePipe;
    SECURITY_ATTRIBUTES sa{sizeof(sa), nullptr, true};
    ULONG uid = UID_INVALID;
    if (CreatePipe(&readPipe, &writePipe, &sa, 0)) {
        // Query the UID of the supplied username.
        std::wstring command = L"/usr/bin/id -u ";
        command += userName;
        int returnValue = 0;
        HANDLE child;
        HRESULT hr = g_wslApi.WslLaunch(command.c_str(), true, GetStdHandle(STD_INPUT_HANDLE), writePipe, GetStdHandle(STD_ERROR_HANDLE), &child);
        if (SUCCEEDED(hr)) {
            // Wait for the child to exit and ensure process exited successfully.
            WaitForSingleObject(child, INFINITE);
            DWORD exitCode;
            if ((GetExitCodeProcess(child, &exitCode) == false) || (exitCode != 0)) {
                hr = E_INVALIDARG;
            }

            CloseHandle(child);
            if (SUCCEEDED(hr)) {
                char buffer[64];
                DWORD bytesRead;

                // Read the output of the command from the pipe and convert to a UID.
                if (ReadFile(readPipe, buffer, (sizeof(buffer) - 1), &bytesRead, nullptr)) {
                    buffer[bytesRead] = ANSI_NULL;
                    try {
                        uid = std::stoul(buffer, nullptr, 10);

                    } catch( ... ) { }
                }
            }
        }

        CloseHandle(readPipe);
        CloseHandle(writePipe);
    }

    return uid;
}
