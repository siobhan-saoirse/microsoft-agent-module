#pragma once
#include <Windows.h>
#include <comdef.h>
#include <thread>
#include <mutex>
#include <queue>
#include <map>
#include <agtctl.h>
#include "AgtSvr_i.c"
#include <tchar.h>
#include "AgtSvr.h"


class AgentInstance
{
public:
    AgentInstance(IAgentEx* ex);
    ~AgentInstance();

    bool Load(const std::wstring& acsPath);

    void Show();
    void Hide();
    long Speak(const std::wstring& text);
    long Think(const std::wstring& text);
    void Play(const std::wstring& anim);
    void MoveTo(long x, long y);
    void GestureAt(long x, long y);

    IAgentCharacterEx* AgentInstance::GetCharacter();

    long Wait(long requestID);
    void NotifyRequestComplete(long requestID);

private:

    std::mutex requestMutex;
    std::queue<long> completedRequests;
};
