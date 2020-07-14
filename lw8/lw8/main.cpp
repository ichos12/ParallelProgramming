#include "pch.h"
#include <iostream>
#include <fstream>
#include <windows.h>
#include <time.h>
#include <string>
#include <vector>
#include <iterator>
#include <mmsystem.h>

#include "main.h"
#include "Pool.h"
#include "Blur.h"
#include "ITask.h"

using namespace std;

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

RGBQuad** readBitMap(const string& fileName, BitMapFileHeader& fileHeader, BitMapInfoHeader& fileInfoHeader)
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

void writeBitMap(const string& fileName, const BitMapFileHeader& fileHeader, const BitMapInfoHeader& fileInfoHeader, RGBQuad** rgbInfo)
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

RGBQuad** blurImage(RGBQuad** defaultRgbInfo, RGBQuad** blurredRgbInfo, BitMapInfoHeader fileInfoHeader, int blocksAmount)
{
	vector<ITask*> tasks;
	ThreadParams* threadParams = new ThreadParams[blocksAmount];
	div_t divider = div(fileInfoHeader.biHeight, blocksAmount);

	int startRow = 0;
	int endRow = divider.quot;
	int remainder = divider.rem;
	for (int i = 0; i < blocksAmount; i++)
	{
		if (remainder != 0)
		{
			endRow++;
			remainder--;
		}

		threadParams[i] = { startRow, endRow, i };

		startRow = endRow;
		endRow += divider.quot;

		tasks.push_back(new Blur(defaultRgbInfo, blurredRgbInfo, fileInfoHeader, threadParams[i]));
	}
	HANDLE* handles = new HANDLE[tasks.size()];

	for (int i = 0; i < tasks.size(); i++)
	{
		handles[i] = CreateThread(NULL, 0, &ThreadProc, tasks[i], CREATE_SUSPENDED, NULL);
		ResumeThread(handles[i]);
	}

	WaitForMultipleObjects(tasks.size(), handles, TRUE, INFINITE);

	return blurredRgbInfo;
}

RGBQuad** blurImageWithPool(RGBQuad** defaultRgbInfo, RGBQuad** blurredRgbInfo, BitMapInfoHeader fileInfoHeader, int blocksAmount, int threadsAmount)
{
	vector<ITask*> tasks;

	ThreadParams* threadParams = new ThreadParams[blocksAmount];
	div_t divider = div(fileInfoHeader.biHeight, blocksAmount);

	int startRow = 0;
	int endRow = divider.quot;
	int remainder = divider.rem;
	for (int i = 0; i < blocksAmount; i++)
	{
		if (remainder != 0)
		{
			endRow++;
			remainder--;
		}

		threadParams[i] = { startRow, endRow, i };

		startRow = endRow;
		endRow += divider.quot;

		tasks.push_back(new Blur(defaultRgbInfo, blurredRgbInfo, fileInfoHeader, threadParams[i]));
	}
	Pool pool(tasks, threadsAmount);
	pool.Execute();

	return blurredRgbInfo;
}

int main(int argc, char *argv[])
{
	if (argc == 2 && strcmp(argv[1], "/?") == 0)
	{
		cout << "Usage: lw8.exe processing_mode(0-base, 1-pool) blocks_amount input_images_dir output_images_dir threads_amount";
		return 0;
	}

	if (argc < 6) {
		cout << "Error arguments number - type /? for usage info" << endl;
		return 0;
	}

	ProcessingMode processingMode;
	if (strcmp(argv[1], "0") == 0)
	{
		processingMode = ProcessingMode::base;
	}
	else if (strcmp(argv[1], "1") == 0)
	{
		processingMode = ProcessingMode::pool;
	}
	else
	{
		throw invalid_argument("Error argument for processing mode");
	}

	int blocksAmount = atoi(argv[2]);
	string inputImgDir = argv[3];
	string outputImgDir = argv[4];
	int threadsAmount = atoi(argv[5]);

	if (!fs::exists(inputImgDir))
	{
		throw exception("This directory does not exist");
	}

	if (!fs::exists(outputImgDir))
	{
		fs::create_directories(outputImgDir);
	}

	fs::recursive_directory_iterator begin(inputImgDir);
	fs::recursive_directory_iterator end;

	vector<fs::path> inputImgPaths;
	vector<fs::path> outputImgPaths;

	copy_if(begin, end, std::back_inserter(inputImgPaths), [](const fs::path& path) {
		return fs::is_regular_file(path) && (path.extension() == ".bmp");
	});

	for (int i = 0; i < inputImgPaths.size(); i++)
	{
		ifstream  input(inputImgPaths[i], ios::binary);

		string outputFileName = outputImgDir + "/out" + to_string(i) + ".bmp";
		outputImgPaths.push_back(outputFileName);
	}

	vector<BitMapFileHeader> fileHeaders;
	vector<BitMapInfoHeader> fileInfoHeaders;
	vector<RGBQuad**> rgbInfos;

	for (size_t i = 0; i < inputImgPaths.size(); i++)
	{
		BitMapFileHeader fileHeader;
		BitMapInfoHeader fileInfoHeader;
		RGBQuad** rgbInfo;

		rgbInfo = readBitMap(inputImgPaths[i].string(), fileHeader, fileInfoHeader);

		fileHeaders.push_back(fileHeader);
		fileInfoHeaders.push_back(fileInfoHeader);
		rgbInfos.push_back(rgbInfo);
	}

	vector<RGBQuad**> blurredImgs;
	if (processingMode == ProcessingMode::base)
	{
		clock_t startTime = clock();
		for (int i = 0; i < fileInfoHeaders.size(); i++)
		{
			RGBQuad** blurredRgbInfo = new RGBQuad*[fileInfoHeaders[i].biHeight];
			for (int j = 0; j < fileInfoHeaders[i].biHeight; j++)
			{
				blurredRgbInfo[j] = new RGBQuad[fileInfoHeaders[i].biWidth];
			}
			RGBQuad** blurredImg = blurImage(rgbInfos[i], blurredRgbInfo, fileInfoHeaders[i], blocksAmount);
			blurredImgs.push_back(blurredImg);
		}
		cout << clock() - startTime << " ms";
	}
	else if (processingMode == ProcessingMode::pool)
	{
		clock_t startTime = clock();
		for (int i = 0; i < fileInfoHeaders.size(); i++)
		{
			RGBQuad** blurredRgbInfo = new RGBQuad*[fileInfoHeaders[i].biHeight];
			for (int j = 0; j < fileInfoHeaders[i].biHeight; j++)
			{
				blurredRgbInfo[j] = new RGBQuad[fileInfoHeaders[i].biWidth];
			}
			RGBQuad** blurredImg = blurImageWithPool(rgbInfos[i], blurredRgbInfo, fileInfoHeaders[i], blocksAmount, threadsAmount);
			blurredImgs.push_back(blurredImg);
		}
		cout << clock() - startTime << " ms";
	}

	for (int i = 0; i < blurredImgs.size(); i++)
	{
		writeBitMap(outputImgPaths[i].string(), fileHeaders[i], fileInfoHeaders[i], blurredImgs[i]);
	}

	return 0;
}