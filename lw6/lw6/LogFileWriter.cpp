#include "pch.h"
#include "LogFileWriter.h"

LogFileWriter::LogFileWriter(std::string fileName) 
	: m_ofstream(fileName) 
{}

void LogFileWriter::Write(std::string value) 
{
	m_ofstream << value << std::endl;
}