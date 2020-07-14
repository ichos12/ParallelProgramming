#include "pch.h"
#include "Pool.h"
#include "Worker.h"

Pool::Pool(std::vector<ITask*> tasks, int threadsAmount)
	: m_threadsAmount(threadsAmount)
	, m_tasks(tasks)
	, m_handles(new HANDLE[m_threadsAmount])
{

	for (int i = 0; i < m_threadsAmount; i++)
	{
		m_handles[i] = CreateThread(NULL, 0, &PoolThreadProc, this, CREATE_SUSPENDED, NULL);
	}
}

void Pool::Execute()
{
	for (int i = 0; i < m_threadsAmount; i++)
	{
		ResumeThread(m_handles[i]);
	}
	WaitForMultipleObjects(m_threadsAmount, m_handles, true, INFINITE);
}

DWORD WINAPI Pool::PoolThreadProc(CONST LPVOID lpParam)
{
	Pool* pool = (Pool*)lpParam;

	while (pool->m_tasks.size() != 0)
	{
		ITask* task = pool->m_tasks.front();
		auto first = pool->m_tasks.cbegin();
		pool->m_tasks.erase(first);
		Worker worker;

		worker.ExecuteTask(task);
	}

	ExitThread(0);
}