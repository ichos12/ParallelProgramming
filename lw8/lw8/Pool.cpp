#include "pch.h"
#include "Pool.h"

Pool::Pool(std::vector<ITask*> tasks, int threadsAmount) 
{
	m_tasksAmount = tasks.size();
	m_handles = new HANDLE[m_tasksAmount];
	for (size_t i = 0; i < m_tasksAmount; i++) 
	{
		m_handles[i] = CreateThread(NULL, 0, &ThreadProc, tasks[i], CREATE_SUSPENDED, NULL);
	}
	m_threadsAmount = threadsAmount;
}

void Pool::Execute() 
{
	size_t count = 0;
	for (size_t i = 0; i < m_tasksAmount; i++) 
	{
		ResumeThread(m_handles[i]);
		count++;
		if (count == m_threadsAmount) 
		{
			WaitForMultipleObjects(i + 1, m_handles, true, INFINITE);
			count = 0;
		}
	}

	WaitForMultipleObjects(m_tasksAmount, m_handles, true, INFINITE);
}