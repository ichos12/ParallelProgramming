#pragma once

#include <windows.h>
#include "ITask.h"

class IWorker
{
public:
	virtual bool ExecuteTask(ITask* task) = 0;
	virtual bool isBusy() = 0;
};