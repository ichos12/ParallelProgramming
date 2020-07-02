#pragma once
#include <iostream>
#include <vector>
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;

#ifndef MAIN_H_INCLUDED
#define MAIN_H_INCLUDED

typedef struct {
	int ciexyzX;
	int ciexyzY;
	int ciexyzZ;
} CieXYZ;

typedef struct {
	CieXYZ  ciexyzRed;
	CieXYZ  ciexyzGreen;
	CieXYZ  ciexyzBlue;
} CieXYZTriple;

// bitmap file header
typedef struct {
	unsigned short bfType;
	unsigned int   bfSize;
	unsigned short bfReserved1;
	unsigned short bfReserved2;
	unsigned int   bfOffBits;
} BitMapFileHeader;

// bitmap info header
typedef struct {
	unsigned int   biSize;
	unsigned int   biWidth;
	unsigned int   biHeight;
	unsigned short biPlanes;
	unsigned short biBitCount;
	unsigned int   biCompression;
	unsigned int   biSizeImage;
	unsigned int   biXPelsPerMeter;
	unsigned int   biYPelsPerMeter;
	unsigned int   biClrUsed;
	unsigned int   biClrImportant;
	unsigned int   biRedMask;
	unsigned int   biGreenMask;
	unsigned int   biBlueMask;
	unsigned int   biAlphaMask;
	unsigned int   biCSType;
	CieXYZTriple   biEndpoints;
	unsigned int   biGammaRed;
	unsigned int   biGammaGreen;
	unsigned int   biGammaBlue;
	unsigned int   biIntent;
	unsigned int   biProfileData;
	unsigned int   biProfileSize;
	unsigned int   biReserved;
} BitMapInfoHeader;

// rgb quad
typedef struct {
	unsigned char  rgbBlue;
	unsigned char  rgbGreen;
	unsigned char  rgbRed;
	unsigned char  rgbReserved;
} RGBQuad;

typedef struct {
	int startRow;
	int endRow;
	int threadNum;
	int blurRadius;
} ThreadParam;

// read bytes
template <typename Type>
void read(std::ifstream &fp, Type &result, std::size_t size) {
	fp.read(reinterpret_cast<char*>(&result), size);
}

// write bytes
template <typename Type>
void write(std::ofstream &fp, Type &info, std::size_t size) {
	fp.write(reinterpret_cast<const char*>(&info), size);
}

// bit extract
unsigned char bitextract(const unsigned int byte, const unsigned int mask);

// processing mode
enum class ProcessingMode
{
	base,
	pool
};

#endif // MAIN_H_INCLUDEDs