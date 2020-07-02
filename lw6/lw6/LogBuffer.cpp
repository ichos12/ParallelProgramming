#include "pch.h"
#include "LogBuffer.h"
#include <exception>

LogBuffer::LogBuffer(LogFileWriter* logFileWriter)
	: m_criticalSection(CRITICAL_SECTION())
	, m_logFileWriter(logFileWriter)
{
	if (!InitializeCriticalSectionAndSpinCount(&m_criticalSection, 0x00000400))
	{
		throw std::exception("Critical section not initialize");
	}

	m_eventHandle = CreateEvent(nullptr, true, false, L"Event");
	m_threadHandle = CreateThread(nullptr, 0, &LogSizeMonitoringThread, (void*)this, 0, nullptr);
}

LogBuffer::~LogBuffer()
{
	CloseHandle(m_eventHandle);
	CloseHandle(m_threadHandle);

	if (&m_criticalSection)
	{
		DeleteCriticalSection(&m_criticalSection);
	}
}

void LogBuffer::AddData(std::string value)
{
	if (&m_criticalSection)
	{
		EnterCriticalSection(&m_criticalSection);
	}

	if (List.GetSize() >= MAX_SIZE)
	{
		SetEvent(m_eventHandle);

		if (WaitForSingleObject(m_threadHandle, INFINITE) == WAIT_OBJECT_0)
		{
			ResetEvent(m_eventHandle);
			m_threadHandle = CreateThread(nullptr, 0, &LogSizeMonitoringThread, (void*)this, 0, nullptr);
		}
	}

	List.Push(value);

	if (&m_criticalSection)
	{
		LeaveCriticalSection(&m_criticalSection);
	}
}

DWORD WINAPI LogBuffer::LogSizeMonitoringThread(const LPVOID lpParam)
{
	LogBuffer* data = (LogBuffer*)lpParam;

	if (WaitForSingleObject(data->m_eventHandle, INFINITE) == WAIT_OBJECT_0)
	{
		auto listSize = data->List.GetSize();

		for (size_t i = 0; i < listSize; i++)
		{
			data->m_logFileWriter->Write(data->List.GetHead());

			data->List.Pop();
		}
	}

	ExitThread(0);
}