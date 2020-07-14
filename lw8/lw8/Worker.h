#pragma once

#include "IWorker.h"

class Worker : public IWorker
{
public:
	~Worker();

	bool ExecuteTask(ITask* task) override;
	bool isBusy() override;

private:
	HANDLE m_handle;
};