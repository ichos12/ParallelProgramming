#pragma once

#include "ITask.h"
#include <vector>

class Pool
{
public:
	Pool(std::vector<ITask*> tasks, int threadsAmount);

	void Execute();

private:
	HANDLE* m_handles;
	size_t m_tasksAmount;
	size_t m_threadsAmount;
};