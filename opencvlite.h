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

// ��ȡԪ�صĴ�С
int ELEMENT_SIZE(int Depth);
// �����ڴ棬32�ֽڶ���
void* AllocMemory(unsigned int size, bool ZeroMemory = false);
// �ͷ��ڴ�
void FreeMemory(void* Ptr);

// ����һ��mat
extern "C" bool _declspec(dllexport) _stdcall CloneMatrix(TMatrix * Src, TMatrix * *Dest);
// ����һ��Mat
extern "C" bool _declspec(dllexport) _stdcall CreateMatrix(int Width, int Height, int Depth, int Channel, TMatrix * *Matrix);
// ����Mat
extern "C" bool _declspec(dllexport) _stdcall FreeMatrix(TMatrix * *Matrix);
// ģ��ƥ�䣨ͬopencv��MatchTemplate��
extern "C" bool _declspec(dllexport) _stdcall MatchTemplate(TMatrix * Src, TMatrix * Template, TMatrix * *Dest);
//��ȡƥ������ͬopencv��MinMaxLoc��
extern "C" bool _declspec(dllexport) _stdcall MinMaxLoc(TMatrix * Src, int& Min_PosX, int& Min_PosY, int& Max_PosX, int& Max_PosY);
