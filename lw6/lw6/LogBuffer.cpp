#include "pch.h"
#include "LogBuffer.h"
#include <exception>

LogBuffer::LogBuffer(LogFileWriter* logFileWriter)
	: m_criticalSection(CRITICAL_SECTION())
	, m_logFileWriter(logFileWriter)
{
	InitializeCriticalSectionAndSpinCount(&m_criticalSection, 0x00000400);

	m_eventHandle = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_threadHandle = CreateThread(NULL, 0, &LogSizeMonitoringThread, this, 0, NULL);
}

LogBuffer::~LogBuffer()
{
	DeleteCriticalSection(&m_criticalSection);
	CloseHandle(m_eventHandle);
	CloseHandle(m_threadHandle);
}

void LogBuffer::AddData(std::string value)
{
	EnterCriticalSection(&m_criticalSection);

	if (List.GetSize() >= MAX_SIZE)
	{
		SetEvent(m_eventHandle);
		ResumeThread(m_threadHandle);
		WaitForSingleObject(m_threadHandle, INFINITE);
		List.Clear();
	}

	List.Push(value);

	LeaveCriticalSection(&m_criticalSection);
}

DWORD WINAPI LogBuffer::LogSizeMonitoringThread(const LPVOID lpParam)
{
	LogBuffer* data = (LogBuffer*)lpParam;
	DWORD dwWaitResult = WAIT_TIMEOUT;
	while (dwWaitResult != WAIT_OBJECT_0) 
	{
		dwWaitResult = WaitForSingleObject(data->m_eventHandle, 1);
	}

	for (int i = 0; i < data->List.GetSize(); i++)
	{
		data->m_logFileWriter->Write(data->List.GetHead());
		data->List.Pop();
	}

	ResetEvent(data->m_eventHandle);
	ExitThread(0);
}