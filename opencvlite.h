#pragma once

#define INT_MIN     (-2147483647 - 1)
#define INT_MAX       2147483647

#define WIDTHBYTES(bytes)  (((bytes * 8) + 31) / 32 * 4)

enum EdgeMode				// Boundary processing methods for some domain algorithms
{
	Tile = 0,				// Repeating edge pixels
	Smear = 1				// Mirrored edge pixels
};

struct TMatrix
{
	int Width;					// The width of a matrix
	int Height;					// The height of a matrix
	int WidthStep;				// The number of bytes occupied by a line element of a matrix
	int Channel;				// Number of matrix channels
	int Depth;					// The type of matrix element
	unsigned char* Data;		// Data of a matrix
	int Reserved;				// Reserved use
};

enum DEPTH
{
	DEPTH_8U = 0,			//	unsigned char
	DEPTH_8S = 1,			//	char
	DEPTH_16S = 2,			//	short
	DEPTH_32S = 3,			//  int
	DEPTH_32F = 4,			//	float
	DEPTH_64F = 5,			//	double
};

// 获取元素的大小
int ELEMENT_SIZE(int Depth);
// 分配内存，32字节对齐
void* AllocMemory(unsigned int size, bool ZeroMemory = false);
// 释放内存
void FreeMemory(void* Ptr);

// 复制一个mat
extern "C" bool _declspec(dllexport) _stdcall CloneMatrix(TMatrix * Src, TMatrix * *Dest);
// 创建一个Mat
extern "C" bool _declspec(dllexport) _stdcall CreateMatrix(int Width, int Height, int Depth, int Channel, TMatrix * *Matrix);
// 销毁Mat
extern "C" bool _declspec(dllexport) _stdcall FreeMatrix(TMatrix * *Matrix);
// 模版匹配（同opencv的MatchTemplate）
extern "C" bool _declspec(dllexport) _stdcall MatchTemplate(TMatrix * Src, TMatrix * Template, TMatrix * *Dest);
//获取匹配结果（同opencv的MinMaxLoc）
extern "C" bool _declspec(dllexport) _stdcall MinMaxLoc(TMatrix * Src, int& Min_PosX, int& Min_PosY, int& Max_PosX, int& Max_PosY);
