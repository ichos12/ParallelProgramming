#include "pch.h"
#include "Pool.h"

Pool::Pool(std::vector<ITask*> tasks, int threadsAmount)
	: m_threadsAmount(threadsAmount)
	, m_tasksAmount(tasks.size())
{
	m_handles = new HANDLE[m_tasksAmount];

	for (int i = 0; i < m_tasksAmount; i++)
	{
		m_handles[i] = CreateThread(NULL, 0, &ThreadProc, tasks[i], CREATE_SUSPENDED, NULL);
	}
}

void Pool::Execute() 
{
	for (int i = 0; i < m_tasksAmount; i++)
	{
		ResumeThread(m_handles[i]);		
		if (WaitForSingleObject(m_handles[i], 1) == WAIT_TIMEOUT)
		{
			if (m_threadsAmount == i + 1)
			{
				WaitForMultipleObjects(i + 1, m_handles, TRUE, INFINITE);
			}
			continue;
		}
	}
}