#pragma once
#include <Windows.h>
#include <string>
#include "List.h"
#include "LogFileWriter.h"

class LogBuffer
{
public:
	LogBuffer(LogFileWriter* logFileWriter);
	~LogBuffer();
	void AddData(std::string value);
private:
	const size_t MAX_SIZE = 500;
	CRITICAL_SECTION m_criticalSection;
	HANDLE m_eventHandle;
	HANDLE m_threadHandle;
	LogFileWriter* m_logFileWriter;
	List List;
	static DWORD WINAPI LogSizeMonitoringThread(CONST LPVOID lpParam);
};