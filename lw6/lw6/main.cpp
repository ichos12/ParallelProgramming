#include "pch.h"
#include <iostream>
#include <fstream>
#include <windows.h>
#include <time.h>
#include <string>
#include <vector>
#include <mmsystem.h>

#include "main.h"
#include "LogFileWriter.h"
#include "LogBuffer.h"

using namespace std;
const int offset = 5;

int g_width;
int g_height;
RGBQuad** g_defaultRgbInfo;
RGBQuad** g_blurredRgbInfo;
clock_t g_startTime;

unsigned char bitextract(const unsigned int byte, const unsigned int mask) {
	if (mask == 0)
	{
		return 0;
	}

	// определение количества нулевых бит справа от маски
	int maskBufer = mask;
	int maskPadding = 0;

	while (!(maskBufer & 1))
	{
		maskBufer >>= 1;
		++maskPadding;
	}

	// применение маски и смещение
	return (byte & mask) >> maskPadding;
}

RGBQuad** readBitMap(char* fileName, BitMapFileHeader& fileHeader, BitMapInfoHeader& fileInfoHeader)
{
	// открываем файл
	ifstream fileStream(fileName, ifstream::binary);
	if (!fileStream)
	{
		cout << "Error opening file '" << fileName << "'." << endl;
		return 0;
	}

	// заголовок изображения
	read(fileStream, fileHeader.bfType, sizeof(fileHeader.bfType));
	read(fileStream, fileHeader.bfSize, sizeof(fileHeader.bfSize));
	read(fileStream, fileHeader.bfReserved1, sizeof(fileHeader.bfReserved1));
	read(fileStream, fileHeader.bfReserved2, sizeof(fileHeader.bfReserved2));
	read(fileStream, fileHeader.bfOffBits, sizeof(fileHeader.bfOffBits));

	if (fileHeader.bfType != 0x4D42)
	{
		cout << "Error: '" << fileName << "' is not BMP file." << endl;
		return 0;
	}

	// информация изображения
	read(fileStream, fileInfoHeader.biSize, sizeof(fileInfoHeader.biSize));

	// bmp core
	if (fileInfoHeader.biSize >= 12)
	{
		read(fileStream, fileInfoHeader.biWidth, sizeof(fileInfoHeader.biWidth));
		read(fileStream, fileInfoHeader.biHeight, sizeof(fileInfoHeader.biHeight));
		read(fileStream, fileInfoHeader.biPlanes, sizeof(fileInfoHeader.biPlanes));
		read(fileStream, fileInfoHeader.biBitCount, sizeof(fileInfoHeader.biBitCount));
	}

	// получаем информацию о битности
	int colorsCount = fileInfoHeader.biBitCount >> 3;
	if (colorsCount < 3)
	{
		colorsCount = 3;
	}

	int bitsOnColor = fileInfoHeader.biBitCount / colorsCount;
	int maskValue = (1 << bitsOnColor) - 1;

	// bmp v1
	if (fileInfoHeader.biSize >= 40)
	{
		read(fileStream, fileInfoHeader.biCompression, sizeof(fileInfoHeader.biCompression));
		read(fileStream, fileInfoHeader.biSizeImage, sizeof(fileInfoHeader.biSizeImage));
		read(fileStream, fileInfoHeader.biXPelsPerMeter, sizeof(fileInfoHeader.biXPelsPerMeter));
		read(fileStream, fileInfoHeader.biYPelsPerMeter, sizeof(fileInfoHeader.biYPelsPerMeter));
		read(fileStream, fileInfoHeader.biClrUsed, sizeof(fileInfoHeader.biClrUsed));
		read(fileStream, fileInfoHeader.biClrImportant, sizeof(fileInfoHeader.biClrImportant));
	}

	// bmp v2
	fileInfoHeader.biRedMask = 0;
	fileInfoHeader.biGreenMask = 0;
	fileInfoHeader.biBlueMask = 0;

	if (fileInfoHeader.biSize >= 52)
	{
		read(fileStream, fileInfoHeader.biRedMask, sizeof(fileInfoHeader.biRedMask));
		read(fileStream, fileInfoHeader.biGreenMask, sizeof(fileInfoHeader.biGreenMask));
		read(fileStream, fileInfoHeader.biBlueMask, sizeof(fileInfoHeader.biBlueMask));
	}

	// если маска не задана, то ставим маску по умолчанию
	if (fileInfoHeader.biRedMask == 0 || fileInfoHeader.biGreenMask == 0 || fileInfoHeader.biBlueMask == 0) {
		fileInfoHeader.biRedMask = maskValue << (bitsOnColor * 2);
		fileInfoHeader.biGreenMask = maskValue << bitsOnColor;
		fileInfoHeader.biBlueMask = maskValue;
	}

	// bmp v3
	if (fileInfoHeader.biSize >= 56)
	{
		read(fileStream, fileInfoHeader.biAlphaMask, sizeof(fileInfoHeader.biAlphaMask));
	}
	else {
		fileInfoHeader.biAlphaMask = maskValue << (bitsOnColor * 3);
	}

	// bmp v4
	if (fileInfoHeader.biSize >= 108)
	{
		read(fileStream, fileInfoHeader.biCSType, sizeof(fileInfoHeader.biCSType));
		read(fileStream, fileInfoHeader.biEndpoints, sizeof(fileInfoHeader.biEndpoints));
		read(fileStream, fileInfoHeader.biGammaRed, sizeof(fileInfoHeader.biGammaRed));
		read(fileStream, fileInfoHeader.biGammaGreen, sizeof(fileInfoHeader.biGammaGreen));
		read(fileStream, fileInfoHeader.biGammaBlue, sizeof(fileInfoHeader.biGammaBlue));
	}

	// bmp v5
	if (fileInfoHeader.biSize >= 124)
	{
		read(fileStream, fileInfoHeader.biIntent, sizeof(fileInfoHeader.biIntent));
		read(fileStream, fileInfoHeader.biProfileData, sizeof(fileInfoHeader.biProfileData));
		read(fileStream, fileInfoHeader.biProfileSize, sizeof(fileInfoHeader.biProfileSize));
		read(fileStream, fileInfoHeader.biReserved, sizeof(fileInfoHeader.biReserved));
	}

	// проверка на поддержку этой версии формата
	if (fileInfoHeader.biSize != 12 && fileInfoHeader.biSize != 40 && fileInfoHeader.biSize != 52 &&
		fileInfoHeader.biSize != 56 && fileInfoHeader.biSize != 108 && fileInfoHeader.biSize != 124) {
		cout << "Error: Unsupported BMP format." << endl;
		return 0;
	}

	if (fileInfoHeader.biBitCount != 16 && fileInfoHeader.biBitCount != 24 && fileInfoHeader.biBitCount != 32) {
		cout << "Error: Unsupported BMP bit count." << endl;
		return 0;
	}

	if (fileInfoHeader.biCompression != 0 && fileInfoHeader.biCompression != 3) {
		cout << "Error: Unsupported BMP compression." << endl;
		return 0;
	}

	// rgb info
	RGBQuad **rgbInfo = new RGBQuad*[fileInfoHeader.biHeight];

	for (unsigned i = 0; i < fileInfoHeader.biHeight; i++)
	{
		rgbInfo[i] = new RGBQuad[fileInfoHeader.biWidth];
	}

	// определение размера отступа в конце каждой строки
	int linePadding = ((fileInfoHeader.biWidth * (fileInfoHeader.biBitCount / 8)) % 4) & 3;

	// чтение
	unsigned int bufer;

	for (unsigned int i = 0; i < fileInfoHeader.biHeight; i++) {
		for (unsigned int j = 0; j < fileInfoHeader.biWidth; j++) {
			read(fileStream, bufer, fileInfoHeader.biBitCount / 8);

			rgbInfo[i][j].rgbRed = bitextract(bufer, fileInfoHeader.biRedMask);
			rgbInfo[i][j].rgbGreen = bitextract(bufer, fileInfoHeader.biGreenMask);
			rgbInfo[i][j].rgbBlue = bitextract(bufer, fileInfoHeader.biBlueMask);
			rgbInfo[i][j].rgbReserved = bitextract(bufer, fileInfoHeader.biAlphaMask);
		}
		fileStream.seekg(linePadding, ios_base::cur);
	}

	return rgbInfo;
}

void writeBitMap(char* fileName, const BitMapFileHeader& fileHeader, const BitMapInfoHeader& fileInfoHeader, RGBQuad** rgbInfo)
{
	ofstream fileStream(fileName, ifstream::binary);

	write(fileStream, fileHeader.bfType, sizeof(fileHeader.bfType));
	write(fileStream, fileHeader.bfSize, sizeof(fileHeader.bfSize));
	write(fileStream, fileHeader.bfReserved1, sizeof(fileHeader.bfReserved1));
	write(fileStream, fileHeader.bfReserved2, sizeof(fileHeader.bfReserved2));
	write(fileStream, fileHeader.bfOffBits, sizeof(fileHeader.bfOffBits));

	write(fileStream, fileInfoHeader.biSize, sizeof(fileInfoHeader.biSize));

	write(fileStream, fileInfoHeader.biWidth, sizeof(fileInfoHeader.biWidth));
	write(fileStream, fileInfoHeader.biHeight, sizeof(fileInfoHeader.biHeight));
	write(fileStream, fileInfoHeader.biPlanes, sizeof(fileInfoHeader.biPlanes));
	write(fileStream, fileInfoHeader.biBitCount, sizeof(fileInfoHeader.biBitCount));

	// bmp v1
	if (fileInfoHeader.biSize >= 40)
	{
		write(fileStream, fileInfoHeader.biCompression, sizeof(fileInfoHeader.biCompression));
		write(fileStream, fileInfoHeader.biSizeImage, sizeof(fileInfoHeader.biSizeImage));
		write(fileStream, fileInfoHeader.biXPelsPerMeter, sizeof(fileInfoHeader.biXPelsPerMeter));
		write(fileStream, fileInfoHeader.biYPelsPerMeter, sizeof(fileInfoHeader.biYPelsPerMeter));
		write(fileStream, fileInfoHeader.biClrUsed, sizeof(fileInfoHeader.biClrUsed));
		write(fileStream, fileInfoHeader.biClrImportant, sizeof(fileInfoHeader.biClrImportant));
	}

	//bmp v2
	if (fileInfoHeader.biSize >= 52)
	{
		write(fileStream, fileInfoHeader.biRedMask, sizeof(fileInfoHeader.biRedMask));
		write(fileStream, fileInfoHeader.biGreenMask, sizeof(fileInfoHeader.biGreenMask));
		write(fileStream, fileInfoHeader.biBlueMask, sizeof(fileInfoHeader.biBlueMask));
	}

	// bmp v3
	if (fileInfoHeader.biSize >= 56)
	{
		write(fileStream, fileInfoHeader.biAlphaMask, sizeof(fileInfoHeader.biAlphaMask));
	}

	// bmp v4
	if (fileInfoHeader.biSize >= 108)
	{
		write(fileStream, fileInfoHeader.biCSType, sizeof(fileInfoHeader.biCSType));
		write(fileStream, fileInfoHeader.biEndpoints, sizeof(fileInfoHeader.biEndpoints));
		write(fileStream, fileInfoHeader.biGammaRed, sizeof(fileInfoHeader.biGammaRed));
		write(fileStream, fileInfoHeader.biGammaGreen, sizeof(fileInfoHeader.biGammaGreen));
		write(fileStream, fileInfoHeader.biGammaBlue, sizeof(fileInfoHeader.biGammaBlue));
	}

	// bmp v5
	if (fileInfoHeader.biSize >= 124)
	{
		write(fileStream, fileInfoHeader.biIntent, sizeof(fileInfoHeader.biIntent));
		write(fileStream, fileInfoHeader.biProfileData, sizeof(fileInfoHeader.biProfileData));
		write(fileStream, fileInfoHeader.biProfileSize, sizeof(fileInfoHeader.biProfileSize));
		write(fileStream, fileInfoHeader.biReserved, sizeof(fileInfoHeader.biReserved));
	}

	for (unsigned i = 0; i < fileInfoHeader.biHeight; i++)
	{
		for (unsigned j = 0; j < fileInfoHeader.biWidth; j++)
		{
			write(fileStream, rgbInfo[i][j].rgbBlue, sizeof(rgbInfo[i][j].rgbBlue));
			write(fileStream, rgbInfo[i][j].rgbGreen, sizeof(rgbInfo[i][j].rgbGreen));
			write(fileStream, rgbInfo[i][j].rgbRed, sizeof(rgbInfo[i][j].rgbRed));
			if (fileInfoHeader.biBitCount == 32)
			{
				write(fileStream, rgbInfo[i][j].rgbReserved, sizeof(rgbInfo[i][j].rgbReserved));
			}
		}
	}
}

DWORD WINAPI ThreadProc(CONST LPVOID lpParam)
{
	ThreadParams* threadParams = (ThreadParams*)lpParam;
	for (unsigned int i = threadParams->startRow; i < threadParams->endRow; ++i)
	{
		for (unsigned int j = 0; j < g_width; ++j)
		{
			unsigned int sumR = 0;
			unsigned int sumG = 0;
			unsigned int sumB = 0;
			unsigned int pixelsAmount = 0;
			for (int di = -offset; di <= offset; ++di)
			{
				for (int dj = -offset; dj <= offset; ++dj)
				{
					unsigned int curI = i + di;
					unsigned int curJ = j + dj;
					if (curI < g_height && curI >= 0 && curJ < g_width && curJ >= 0)
					{
						sumR += g_defaultRgbInfo[curI][curJ].rgbRed;
						sumG += g_defaultRgbInfo[curI][curJ].rgbGreen;
						sumB += g_defaultRgbInfo[curI][curJ].rgbBlue;
						++pixelsAmount;
					}

				}
			}

			g_blurredRgbInfo[i][j].rgbRed = sumR / pixelsAmount;
			g_blurredRgbInfo[i][j].rgbGreen = sumG / pixelsAmount;
			g_blurredRgbInfo[i][j].rgbBlue = sumB / pixelsAmount;

			clock_t elapsedTime = clock() - g_startTime;
			string value = to_string(threadParams->threadNum + 1) + " " + to_string((float)elapsedTime / CLOCKS_PER_SEC);
			threadParams->logBuffer->AddData(value);
		}
	}
	ExitThread(0);
}

RGBQuad** Blur(RGBQuad** rgbInfo, BitMapInfoHeader& fileInfoHeader, int threadsAmount, int coresAmount, vector<ThreadPriority> threadsPriority)
{
	g_width = fileInfoHeader.biWidth;
	g_height = fileInfoHeader.biHeight;
	g_defaultRgbInfo = rgbInfo;
	g_blurredRgbInfo = new RGBQuad*[g_height];
	for (unsigned i = 0; i < g_height; i++)
	{
		g_blurredRgbInfo[i] = new RGBQuad[g_width];
	}

	// создание потоков
	HANDLE* handles = new HANDLE[threadsAmount];
	ThreadParams* threadParams = new ThreadParams[threadsAmount];

	LogFileWriter* writer = new LogFileWriter("log.txt");
	LogBuffer* logBuffer = new LogBuffer(writer);

	div_t divider = div(g_height, threadsAmount);

	int startRow = 0;
	int endRow = divider.quot - 1;
	int remainder = divider.rem;
	for (int i = 0; i < threadsAmount; i++)
	{
		if (remainder != 0)
		{
			endRow++;
			remainder--;
		}

		threadParams[i] = { startRow, endRow, i, logBuffer };

		startRow = endRow + 1;
		endRow += divider.quot;
	}

	for (int i = 0; i < threadsAmount; i++)
	{

		handles[i] = CreateThread(NULL, 0, &ThreadProc, &threadParams[i], CREATE_SUSPENDED, NULL);
		SetThreadAffinityMask(handles[i], (1 << coresAmount) - 1);
		SetThreadPriority(handles[i], threadsPriority[i]);

	}

	for (int i = 0; i < threadsAmount; i++)
	{
		ResumeThread(handles[i]);
	}

	// ожидание окончания работы потоков
	WaitForMultipleObjects(threadsAmount, handles, true, INFINITE);

	return g_blurredRgbInfo;
}


int main(int argc, char *argv[])
{
	if (argc == 2 && strcmp(argv[1], "/?") == 0)
	{
		cout << "Usage: lw6.exe infile_name outfile_name threads_amount cores_amount priorities(separated by space)\n Example priorities:\n below_normal(-1) \n normal(0) \n above_normal(1)";
		return 0;
	}

	if (argc < 5) {
		cout << "Error arguments number - type /? for usage info" << endl;
		return 0;
	}

	char *inFileName = argv[1];
	char *outFileName = argv[2];
	int threadsAmount = atoi(argv[3]);
	int coresAmount = atoi(argv[4]);

	vector<ThreadPriority> threadsPriority;

	for (int i = 5; i < 5 + threadsAmount; i++)
	{
		if (strcmp(argv[i], "below_normal") == 0 || strcmp(argv[i], "-1") == 0)
		{
			threadsPriority.push_back(ThreadPriority::priority_below_normal);
			cout << "below\n";
		}
		else if (strcmp(argv[i], "normal") == 0 || strcmp(argv[i], "0") == 0) {
			threadsPriority.push_back(ThreadPriority::priority_normal);
			cout << "normal\n";
		}
		else if (strcmp(argv[i], "above_normal") == 0 || strcmp(argv[i], "1") == 0) {
			threadsPriority.push_back(ThreadPriority::priority_above_normal);
			cout << "above\n";
		}
	}

	BitMapFileHeader fileHeader;
	BitMapInfoHeader fileInfoHeader;

	RGBQuad** inputImg = readBitMap(inFileName, fileHeader, fileInfoHeader);

	clock_t g_startTime = clock();
	RGBQuad** blurredImg = Blur(inputImg, fileInfoHeader, threadsAmount, coresAmount, threadsPriority);

	writeBitMap(outFileName, fileHeader, fileInfoHeader, blurredImg);
	return 0;
}
