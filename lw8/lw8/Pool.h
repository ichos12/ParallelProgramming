#pragma once

#include "ITask.h"
#include "Worker.h"
#include <vector>

class Pool
{
public:
	Pool(std::vector<ITask*> tasks, int threadsAmount);

	void Execute();

	static DWORD WINAPI PoolThreadProc(CONST LPVOID lpParam);
private:
	size_t m_threadsAmount;
	HANDLE* m_handles;
	std::vector<Worker> m_workers;
	std::vector<ITask*> m_tasks;
};