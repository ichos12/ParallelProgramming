#pragma once

#include <windows.h>

class ITask
{
public:
	virtual void Execute() = 0;
};

static DWORD WINAPI ThreadProc(CONST LPVOID lpParam) {
	ITask* task = (ITask*)lpParam;
	task->Execute();
	ExitThread(0);
}