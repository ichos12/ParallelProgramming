#include "pch.h"
#include "Blur.h"

Blur::Blur(RGBQuad** defaultRgbInfo, RGBQuad** blurredRgbInfo, BitMapInfoHeader& fileInfoHeader, int startRow, int endRow)
	: m_defaultRgbInfo(defaultRgbInfo)
	, m_blurredRgbInfo(blurredRgbInfo)
	, m_fileInfoHeader(fileInfoHeader)
	, m_startRow(startRow)
	, m_endRow(endRow)
{
}

void Blur::Execute()
{
	for (unsigned int i = m_startRow; i < m_endRow; ++i)
	{
		for (unsigned int j = 0; j < m_fileInfoHeader.biWidth; ++j)
		{
			unsigned int sumR = 0;
			unsigned int sumG = 0;
			unsigned int sumB = 0;
			unsigned int pixelsAmount = 0;
			for (int di = -blurRadius; di <= blurRadius; ++di)
			{
				for (int dj = -blurRadius; dj <= blurRadius; ++dj)
				{
					unsigned int curI = i + di;
					unsigned int curJ = j + dj;
					if (curI < m_fileInfoHeader.biHeight && curI >= 0 && curJ < m_fileInfoHeader.biWidth && curJ >= 0)
					{
						sumR += m_defaultRgbInfo[curI][curJ].rgbRed;
						sumG += m_defaultRgbInfo[curI][curJ].rgbGreen;
						sumB += m_defaultRgbInfo[curI][curJ].rgbBlue;
						++pixelsAmount;
					}

				}
			}

			m_blurredRgbInfo[i][j].rgbRed = sumR / pixelsAmount;
			m_blurredRgbInfo[i][j].rgbGreen = sumG / pixelsAmount;
			m_blurredRgbInfo[i][j].rgbBlue = sumB / pixelsAmount;
		}
	}
}