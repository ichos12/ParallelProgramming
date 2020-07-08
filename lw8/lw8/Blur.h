#pragma once

#include "ITask.h"
#include "main.h"

class Blur : public ITask
{
public:
	Blur(RGBQuad** defaultRgbInfo, RGBQuad** blurredRgbInfo, BitMapInfoHeader& fileInfoHeader, ThreadParams& threadParams);

	void Execute() override;

private:
	const int blurRadius = 10;

	RGBQuad** m_defaultRgbInfo;
	RGBQuad** m_blurredRgbInfo;
	BitMapInfoHeader m_fileInfoHeader;
	ThreadParams m_threadParams;
};