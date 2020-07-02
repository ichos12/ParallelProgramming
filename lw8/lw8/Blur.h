#pragma once

#include "ITask.h"
#include "main.h"

class Blur : public ITask
{
public:
	Blur(RGBQuad** defaultRgbInfo, RGBQuad** blurredRgbInfo, BitMapInfoHeader& fileInfoHeader, int startRow, int endRow);

	void Execute() override;

private:
	const int blurRadius = 5;

	RGBQuad** m_defaultRgbInfo;
	RGBQuad** m_blurredRgbInfo;
	BitMapInfoHeader m_fileInfoHeader;
	int m_startRow;
	int m_endRow;
};