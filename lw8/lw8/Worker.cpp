#include "pch.h"
#include "Worker.h"

Worker::~Worker()
{
	WaitForSingleObject(m_handle, INFINITE);
}

bool Worker::ExecuteTask(ITask* task)
{
	if (isBusy())
	{
		return false;
	}
	m_handle = CreateThread(NULL, 0, &ThreadProc, task, 0, NULL);
	return true;
}

bool Worker::isBusy()
{
	if (GetExitCodeThread(m_handle, (LPDWORD)STILL_ACTIVE))
	{
		return true;
	}
	return false;
}