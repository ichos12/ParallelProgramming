#pragma once
#include <fstream>
#include <string>

class LogFileWriter
{
public:
	LogFileWriter(std::string fileName);
	void Write(std::string value);
private:
	std::ofstream m_ofstream;
};
