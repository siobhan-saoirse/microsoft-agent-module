#include "agent.h"
#include <comutil.h>

#pragma comment(lib, "comsuppw.lib")

static const wchar_t* kpszAppTitle = L"BonziBuddy";
IAgentEx *g_agentEx;
IAgentCharacterEx* pCharacterEx = NULL;
extern long	lCharID;
extern long	lRequestID;

AgentInstance::AgentInstance(IAgentEx *ex) {
    g_agentEx = ex;
}

AgentInstance::~AgentInstance()
{
    if (pCharacterEx)
    {
        pCharacterEx->Release();
        pCharacterEx = nullptr;
    }
}

bool AgentInstance::Load(const std::wstring& acsPath)
{
    // Get IAgentEx (exactly like Hello.cpp)
    IAgentEx* agentEx = g_agentEx;

    // Load pCharacterEx
    VARIANT vPath;
    VariantInit(&vPath);
    vPath.vt = VT_BSTR;
    vPath.bstrVal = SysAllocString(acsPath.c_str());

    HRESULT hr = agentEx->Load(vPath, &lCharID, &lRequestID);
    VariantClear(&vPath);

    if (FAILED(hr))
    {
        MessageBoxW(nullptr, acsPath.c_str(),
            L"Failed to load pCharacterEx", MB_ICONERROR);
        return false;
    }

    // Get pCharacterEx interface
    agentEx->GetCharacterEx(lCharID, &pCharacterEx);
    pCharacterEx->SetLanguageID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));

    return SUCCEEDED(hr) && pCharacterEx != nullptr;
}

IAgentCharacterEx* AgentInstance::GetCharacter()
{
    return pCharacterEx;
}

void AgentInstance::Show()
{
    if (!pCharacterEx)
        return;

    VARIANT v;
    VariantInit(&v);
    v.vt = VT_ERROR;
    v.scode = DISP_E_PARAMNOTFOUND;

    pCharacterEx->Show(FALSE, &lRequestID);
    VariantClear(&v);
}

void AgentInstance::Hide()
{
    if (!pCharacterEx)
        return;

    VARIANT v;
    VariantInit(&v);
    v.vt = VT_ERROR;
    v.scode = DISP_E_PARAMNOTFOUND;

    pCharacterEx->Hide(FALSE, &lRequestID);
    VariantClear(&v);
}

long AgentInstance::Speak(const std::wstring& text)
{
    if (!pCharacterEx)
        return -1;

    // Speak expects: BSTR bszText, BSTR bszUrl, long * plRequestID
    // So we need to pass BSTR, not VARIANT

    BSTR bszSpeak = SysAllocString(text.c_str());

    _bstr_t bstrEmpty(L"");

    HRESULT hr = pCharacterEx->Speak(bszSpeak, bstrEmpty, &lRequestID);

    SysFreeString(bszSpeak);

    if (FAILED(hr))
        return -1;

    return lRequestID;
}

long AgentInstance::Think(const std::wstring& text)
{
    if (!pCharacterEx)
        return -1;

    // Speak expects: BSTR bszText, BSTR bszUrl, long * plRequestID
    // So we need to pass BSTR, not VARIANT

    BSTR bszSpeak = SysAllocString(text.c_str());

    _bstr_t bstrEmpty(L"");

    HRESULT hr = pCharacterEx->Think(bszSpeak, &lRequestID);

    SysFreeString(bszSpeak);

    if (FAILED(hr))
        return -1;

    return lRequestID;
}

void AgentInstance::Play(const std::wstring& anim)
{
    if (!pCharacterEx)
        return;

    pCharacterEx->Play(_bstr_t(anim.c_str()), &lRequestID);
}

void AgentInstance::MoveTo(long x, long y)
{
    if (!pCharacterEx)
        return;

    VARIANT v;
    VariantInit(&v);
    v.vt = VT_ERROR;
    v.scode = DISP_E_PARAMNOTFOUND;

    pCharacterEx->MoveTo(x, y, 0, &lRequestID); // Pass by reference
    VariantClear(&v);
}

void AgentInstance::GestureAt(long x, long y)
{
    if (!pCharacterEx)
        return;

    pCharacterEx->GestureAt(x, y, &lRequestID);
}

long AgentInstance::Wait(long requestID)
{
    for (;;)
    {
        {
            std::lock_guard<std::mutex> lock(requestMutex);
            if (!completedRequests.empty() &&
                completedRequests.front() == requestID)
            {
                completedRequests.pop();
                return requestID;
            }
        }
        Sleep(10);
    }
}

void AgentInstance::NotifyRequestComplete(long requestID)
{
    std::lock_guard<std::mutex> lock(requestMutex);
    completedRequests.push(requestID);
}
