#include <Windows.h>
#include <GarrysMod/Lua/Interface.h>

BOOL APIENTRY DllMain(HMODULE hModule,
                      DWORD  ul_reason_for_call,
                      LPVOID lpReserved)
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
        CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    if (ul_reason_for_call == DLL_PROCESS_DETACH)
        CoUninitialize();

    return TRUE;
}