#include <GarrysMod/Lua/Interface.h>
#include <stdint.h>
#include <string>
#include "agent.h"
#include "agtctl.h"
#include "agtctl_i.c"
#include <thread>
#include <atomic>
#include <Windows.h>

using namespace GarrysMod::Lua;

static AgentInstance* g_agent = nullptr;
static std::atomic<bool> g_running(false);
static std::thread g_msgThread;
static IAgentCtl* g_agentCtl = nullptr;
static IAgentEx* g_agentEx = nullptr;
static bool g_comInitialized = false;
long	lCharID;
long	lRequestID;
extern IAgentCharacterEx* pCharacterEx;

static bool EnsureAgentServer()
{
    if (g_agentEx)
        return true;

    if (!g_comInitialized)
    {
        HRESULT hr = CoInitialize(NULL);
        if (FAILED(hr))
            return false;
        g_comInitialized = true;
    }

    HRESULT hr = CoCreateInstance(
        CLSID_AgentServer,
        NULL,
        CLSCTX_SERVER,
        IID_IAgentEx,
        (void**)&g_agentEx
    );

    if (FAILED(hr)) {

        _TCHAR				szError[256];
        wsprintf(szError, _T("There was an error initializing Microsoft Agent, code = 0x%x"), hr);

        MessageBox(NULL,
            szError,
            "BonziBuddy",
            MB_OK | MB_ICONERROR | MB_TOPMOST);

        CoUninitialize();

        return false;
    }

    return true;
}


// Background message loop for COM events
static void MessageLoopThread()
{
    MSG msg;
    while (g_running)
    {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        Sleep(10);
    }
}
std::wstring ToWide(const char* str)
{
    if (!str) return L"";

    int len = MultiByteToWideChar(
        CP_UTF8,        // Lua strings are UTF-8
        0,
        str,
        -1,
        nullptr,
        0
    );

    std::wstring out(len - 1, L'\0');
    MultiByteToWideChar(
        CP_UTF8,
        0,
        str,
        -1,
        &out[0],
        len
    );

    return out;
}

// -------------------- Lua functions --------------------

LUA_FUNCTION(magent_create)
{
    const char* path = LUA->CheckString(1);

    if (!EnsureAgentServer())
    {
        LUA->PushBool(false);
        return 1;
    }

    delete g_agent;
    g_agent = new AgentInstance(g_agentEx);

    std::wstring wpath = ToWide(path);
    VARIANT				vPath;
    VariantInit(&vPath);
    vPath.vt = VT_BSTR;
    std::wstring fullPath = L"\\Windows\\msagent\\chars\\" + wpath + L".acs";
    vPath.bstrVal = SysAllocString(fullPath.c_str());

    bool loaded = g_agent->Load(fullPath);

    LUA->PushBool(loaded);

    if (!g_running)
    {
        g_running = true;
        g_msgThread = std::thread(MessageLoopThread);
    }

    return 1;
}


LUA_FUNCTION(magent_show)
{
    if (g_agent) g_agent->Show();
    return 0;
}

LUA_FUNCTION(magent_hide)
{
    if (g_agent) g_agent->Hide();
    return 0;
}

LUA_FUNCTION(magent_speak)
{
    const char* text = LUA->CheckString(1);
    if (g_agent)
        g_agent->Speak(std::wstring(text, text + strlen(text)));
    return 0;
}

LUA_FUNCTION(magent_think)
{
    const char* text = LUA->CheckString(1);
    if (g_agent)
        g_agent->Think(std::wstring(text, text + strlen(text))); 
    return 0;
}

LUA_FUNCTION(magent_play)
{
    const char* anim = LUA->CheckString(1);
    if (g_agent)
        g_agent->Play(std::wstring(anim, anim + strlen(anim)));
    return 0;
}

LUA_FUNCTION(magent_move_to)
{
    // skip self if called as obj:MoveTo(x, y)
    int argOffset = 1;
    if (LUA->IsType(1, GarrysMod::Lua::Type::Table))
        argOffset = 2;

    long x = LUA->CheckNumber(argOffset);
    long y = LUA->CheckNumber(argOffset + 1);
    if (g_agent)
        g_agent->MoveTo(x, y);
    return 0;
}

LUA_FUNCTION(magent_gesture_at)
{
    long x = LUA->CheckNumber(1);
    long y = LUA->CheckNumber(2);
    if (g_agent)
        g_agent->GestureAt(x, y);
    return 0;
}

// -------------------- Module setup --------------------

GMOD_MODULE_OPEN()
{
    LUA->PushSpecial(GarrysMod::Lua::SPECIAL_GLOB);
    LUA->CreateTable();
    LUA->PushCFunction(magent_create);      LUA->SetField(-2, "Create");
    LUA->PushCFunction(magent_show);        LUA->SetField(-2, "Show");
    LUA->PushCFunction(magent_hide);        LUA->SetField(-2, "Hide");
    LUA->PushCFunction(magent_speak);       LUA->SetField(-2, "Speak");
    LUA->PushCFunction(magent_think);       LUA->SetField(-2, "Think");
    LUA->PushCFunction(magent_play);        LUA->SetField(-2, "Play");
    LUA->PushCFunction(magent_move_to);     LUA->SetField(-2, "MoveTo");
    LUA->PushCFunction(magent_gesture_at);  LUA->SetField(-2, "GestureAt");
    LUA->SetField(-2, "msagent");
    LUA->Pop();

    return 0;
}

GMOD_MODULE_CLOSE()
{
    // Hide agent before destroying everything
    if (pCharacterEx)
    {
        long hideReq = 0;
        pCharacterEx->Hide(false, &hideReq);
        Sleep(5000);
    }

    delete g_agent;
    g_agent = nullptr;

    if (g_agentEx)
    {
        g_agentEx->Release();
        g_agentEx = nullptr;
    }

    if (g_agentCtl)
    {
        g_agentCtl->Release();
        g_agentCtl = nullptr;
    }

    g_running = false;
    if (g_msgThread.joinable())
        g_msgThread.join();

    if (g_comInitialized)
    {
        CoUninitialize();
        g_comInitialized = false;
    }

    return 0;
}

