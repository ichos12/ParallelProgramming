#include "pch.h"
#include "Worker.h"
#include <iostream>

Worker::Worker()
{
	WaitForSingleObject(m_handle, INFINITE);
}

bool Worker::ExecuteTask(ITask* task)
{
	if (IsBusy())
	{
		return false;
	}
	m_handle = CreateThread(NULL, 0, &ThreadProc, task, CREATE_SUSPENDED, NULL);
	ResumeThread(m_handle);
	return true;
}

bool Worker::IsBusy() const
{
	if (m_handle == nullptr)
	{
		return false;
	}

	LPDWORD code = new DWORD;
	GetExitCodeThread(m_handle, code);

	if (*code == STILL_ACTIVE)
	{
		return true;
	}

	return false;
}

void Worker::Wait() const
{
	WaitForSingleObject(m_handle, INFINITE);
}