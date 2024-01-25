#include "pch.h"
#include "opencvlite.h"

/// <summary>
/// 根据矩阵元素的类型，获取元素实际占用的字节数。
/// </summary>
/// <param name="Depth">mat类型</param>
/// <returns></returns>
int ELEMENT_SIZE(int Depth)
{
	int Size;
	switch (Depth)
	{
	case DEPTH_8U:
		Size = sizeof(unsigned char);
		break;
	case DEPTH_8S:
		Size = sizeof(char);
		break;
	case DEPTH_16S:
		Size = sizeof(short);
		break;
	case DEPTH_32S:
		Size = sizeof(int);
		break;
	case DEPTH_32F:
		Size = sizeof(float);
		break;
	case DEPTH_64F:
		Size = sizeof(double);
		break;
	default:
		Size = 0;
		break;
	}
	return Size;
}

void* AllocMemory(unsigned int Size, bool ZeroMemory)
{
	void* Ptr = _mm_malloc(Size, 32);
	if (Ptr != NULL)
		if (ZeroMemory == true)
			memset(Ptr, 0, Size);
	return Ptr;
}

void FreeMemory(void* Ptr)
{
	if (Ptr != NULL) _mm_free(Ptr);
}

/// <summary>
/// 创建新的矩阵数据
/// </summary>
/// <param name="Width">矩阵的宽度</param>
/// <param name="Height">矩阵的高度</param>
/// <param name="Depth">矩阵的颜色深度</param>
/// <param name="Channel">矩阵的通道数</param>
/// <param name="Matrix">返回的Matrix对象</param>
/// <returns>如果成功则返回true，否则返回false</returns>
bool _declspec(dllexport) _stdcall CreateMatrix(int Width, int Height, int Depth, int Channel, TMatrix** Matrix)
{
	if (Width < 1 || Height < 1)
		//参数有问题
		return false;
	if (Depth != DEPTH_8U && Depth != DEPTH_8S && Depth != DEPTH_16S && Depth != DEPTH_32S && Depth != DEPTH_32F && Depth != DEPTH_64F)
		//参数有问题
		return false;
	if (Channel != 1 && Channel != 2 && Channel != 3 && Channel != 4)
		//参数有问题
		return false;
	*Matrix = (TMatrix*)AllocMemory(sizeof(TMatrix));
	(*Matrix)->Width = Width;
	(*Matrix)->Height = Height;
	(*Matrix)->Depth = Depth;
	(*Matrix)->Channel = Channel;
	(*Matrix)->WidthStep = WIDTHBYTES(Width * Channel * ELEMENT_SIZE(Depth));
	(*Matrix)->Data = (unsigned char*)AllocMemory((*Matrix)->Height * (*Matrix)->WidthStep, true);
	if ((*Matrix)->Data == NULL)
	{
		FreeMemory(*Matrix);
		//内存不够
		return false;
	}
	(*Matrix)->Reserved = 0;
	return true;
}

bool _declspec(dllexport) _stdcall FreeMatrix(TMatrix** Matrix)
{
	if ((*Matrix) == NULL)
		return true;
	if ((*Matrix)->Data == NULL)
	{
		FreeMemory((*Matrix));
		return true;
	}
	else
	{
		FreeMemory((*Matrix)->Data);
		FreeMemory((*Matrix));
		return true;
	}
}

bool _declspec(dllexport) _stdcall CloneMatrix(TMatrix* Src, TMatrix** Dest)
{
	if (Src == NULL) return false;
	if (Src->Data == NULL) return false;
	bool Ret = CreateMatrix(Src->Width, Src->Height, Src->Depth, Src->Channel, Dest);
	if (Ret) memcpy((*Dest)->Data, Src->Data, (*Dest)->Height * (*Dest)->WidthStep);
	return Ret;
}

/// <summary>
/// 计算图像的局部平方和，优化了速度，支持1和3通道图像。
/// </summary>
/// <param name="Src">平方和的源图像</param>
/// <param name="Dest">平方和数据，需要使用int型矩阵保存，大小为Src-＞Width-SizeX+1，Src-＞Height-SizeY+1，数据在程序内部分配</param>
/// <param name="SizeX">如果是半径模式，则在水平方向上使用的模板的大小对应于2*radius+1。</param>
/// <param name="SizeY">如果是半径模式，则垂直方向上使用的模板的大小对应于2*radius+1</param>
/// <remarks> 1:An optimization algorithm similar to BoxBlur is used.</remarks>
/// <returns></returns>
bool GetLocalSquareSum(TMatrix* Src, TMatrix** Dest, int SizeX, int SizeY)
{
	if (Src == NULL || Src->Data == NULL)
		return false;
	if (Src->Depth != DEPTH_8U || Src->Channel == 4)
		return false;
	if (SizeX < 0 || SizeY < 0)
		return false;

	int X, Y, Z, SrcW, SrcH, DestW, DestH, LastIndex, NextIndex, Sum;
	int* ColSum{}, * LinePD;
	unsigned char* SamplePS, * LastAddress, * NextAddress;


	SrcW = Src->Width, SrcH = Src->Height;
	DestW = SrcW - SizeX + 1, DestH = SrcH - SizeY + 1;

	bool Ret = CreateMatrix(DestW, DestH, DEPTH_32S, 1, Dest);
	if (!Ret)
		goto Done;
	ColSum = (int*)AllocMemory(SrcW * sizeof(int), true);
	if (ColSum == NULL) {
		Ret = false;
		goto Done;
	}

	if (Src->Channel == 1)
	{
		for (Y = 0; Y < DestH; Y++)
		{
			LinePD = (int*)((*Dest)->Data + Y * (*Dest)->WidthStep);
			if (Y == 0)
			{
				for (X = 0; X < SrcW; X++)
				{
					Sum = 0;
					for (Z = 0; Z < SizeY; Z++)
					{
						SamplePS = Src->Data + Z * Src->WidthStep + X;
						Sum += SamplePS[0] * SamplePS[0];
					}
					ColSum[X] = Sum;
				}
			}
			else
			{
				LastAddress = Src->Data + (Y - 1) * Src->WidthStep;
				NextAddress = Src->Data + (Y + SizeY - 1) * Src->WidthStep;
				for (X = 0; X < SrcW; X++)
				{
					ColSum[X] -= LastAddress[X] * LastAddress[X] - NextAddress[X] * NextAddress[X];
				}
			}
			for (X = 0; X < DestW; X++)
			{
				if (X == 0)
				{
					Sum = 0;
					for (Z = 0; Z < SizeX; Z++)
					{
						Sum += ColSum[Z];
					}
				}
				else
				{
					Sum -= ColSum[X - 1] - ColSum[X + SizeX - 1];
				}
				LinePD[X] = Sum;
			}
		}
	}
	else if (Src->Channel == 3)
	{
		for (Y = 0; Y < DestH; Y++)
		{
			LinePD = (int*)((*Dest)->Data + Y * (*Dest)->WidthStep);
			if (Y == 0)
			{
				for (X = 0; X < SrcW; X++)
				{
					Sum = 0;
					for (Z = 0; Z < SizeY; Z++)
					{
						SamplePS = Src->Data + Z * Src->WidthStep + X * 3;			//	Three channels are added together
						Sum += SamplePS[0] * SamplePS[0] + SamplePS[1] * SamplePS[1] + SamplePS[2] * SamplePS[2];
					}
					ColSum[X] = Sum;
				}
			}
			else
			{
				LastAddress = Src->Data + (Y - 1) * Src->WidthStep;
				NextAddress = Src->Data + (Y + SizeY - 1) * Src->WidthStep;
				for (X = 0; X < SrcW; X++)
				{
					ColSum[X] += NextAddress[0] * NextAddress[0] + NextAddress[1] * NextAddress[1] + NextAddress[2] * NextAddress[2] - LastAddress[0] * LastAddress[0] - LastAddress[1] * LastAddress[1] - LastAddress[2] * LastAddress[2];
					LastAddress += 3;
					NextAddress += 3;
				}
			}
			for (X = 0; X < DestW; X++)
			{
				if (X == 0)
				{
					Sum = 0;
					for (Z = 0; Z < SizeX; Z++)
					{
						Sum += ColSum[Z];
					}
				}
				else
				{
					Sum -= ColSum[X - 1] - ColSum[X + SizeX - 1];
				}
				LinePD[X] = Sum;
			}
		}
	}
Done:
	FreeMemory(ColSum);
	return Ret;
}

/// <summary>
/// 基于SSE的字节数据的乘法运算。
/// </summary>
/// <param name="Kernel">需要卷积的矩阵</param>
/// <param name="Conv">卷积矩阵</param>
/// <param name="Length">数组中所有元素的长度</param>
/// <remarks> 1: SSE optimization is used.</remarks>
///	<remarks> https://msdn.microsoft.com/en-us/library/t5h7783k(v=vs.90).aspx </remarks>
/// <returns></returns>
int MultiplySSE(unsigned char* Kernel, unsigned char* Conv, int Length)
{
	int Y, Sum;
	__m128i vsum = _mm_set1_epi32(0);
	__m128i vk0 = _mm_set1_epi8(0);
	for (Y = 0; Y <= Length - 16; Y += 16)
	{
		__m128i v0 = _mm_loadu_si128((__m128i*)(Kernel + Y));				// Corresponding to the movdqu instruction, no 16 byte alignment is required
		__m128i v0l = _mm_unpacklo_epi8(v0, vk0);
		__m128i v0h = _mm_unpackhi_epi8(v0, vk0);							//	The purpose of these two lines is to load them into two 128-bit registers for the following 16-bit SSE function call of _mm_madd_epi16 (the role of vk0 is mainly to set the upper 8 bits to 0)				
		__m128i v1 = _mm_loadu_si128((__m128i*)(Conv + Y));
		__m128i v1l = _mm_unpacklo_epi8(v1, vk0);
		__m128i v1h = _mm_unpackhi_epi8(v1, vk0);
		vsum = _mm_add_epi32(vsum, _mm_madd_epi16(v0l, v1l));				//	With _mm_madd_epi16 it is possible to multiply eight 16-bit numbers at a time and then add the results of the two 16 to a 32 number, r0: = (a0 * b0) + (a1 * b1) https://msdn.microsoft.com/en-us/library/yht36sa6(v=vs.90).aspx
		vsum = _mm_add_epi32(vsum, _mm_madd_epi16(v0h, v1h));
	}
	for (; Y <= Length - 8; Y += 8)
	{
		__m128i v0 = _mm_loadl_epi64((__m128i*)(Kernel + Y));
		__m128i v0l = _mm_unpacklo_epi8(v0, vk0);
		__m128i v1 = _mm_loadl_epi64((__m128i*)(Conv + Y));
		__m128i v1l = _mm_unpacklo_epi8(v1, vk0);
		vsum = _mm_add_epi32(vsum, _mm_madd_epi16(v0l, v1l));
	}
	vsum = _mm_add_epi32(vsum, _mm_srli_si128(vsum, 8));
	vsum = _mm_add_epi32(vsum, _mm_srli_si128(vsum, 4));
	Sum = _mm_cvtsi128_si32(vsum);											// MOVD function, Moves the least significant 32 bits of a to a 32-bit integer：   r := a0

	for (; Y < Length; Y++)
	{
		Sum += Kernel[Y] * Conv[Y];
	}
	return Sum;
}

/// <summary>
/// 基于SSE的图像卷积算法
/// </summary>
/// <param name="Src">需要处理的源映像的数据结构</param>
/// <param name="Conv">卷积矩阵，必须是图像数据</param>
/// <param name="Dest">保存的卷积结果的数据结构必须是int矩阵</param>
/// <remarks> 1: SSE optimization is used.</remarks>
/// <remarks> 2: The SSE multiplication of the byte array is much faster than the ordinary SSE speed.</remarks>
/// <remarks> 3: Testing using FFT will be slower. </remarks>
/// <returns></returns>
bool FastConv2(TMatrix* Src, TMatrix* Conv, TMatrix** Dest)
{
	if (Src == NULL || Conv == NULL)
		return false;
	if (Src->Data == NULL || Conv->Data == NULL)
		return false;
	if (Src->Channel != Conv->Channel || Src->Depth != Conv->Depth)
		return false;
	if (Src->Depth != DEPTH_8U || Src->Channel == 4)
		return false;

	int X, Y, Length, * LinePD;
	int SrcW, SrcH, DestW, DestH, ConvW, ConvH;
	unsigned char* LinePS, * CurKer, * Conv16{}, * Kernel{};

	SrcW = Src->Width, SrcH = Src->Height, ConvW = Conv->Width, ConvH = Conv->Height;
	DestW = SrcW - ConvW + 1, DestH = SrcH - ConvH + 1, Length = ConvW * ConvH * Src->Channel;

	bool Ret = CreateMatrix(DestW, DestH, DEPTH_32S, 1, Dest);
	if (!Ret)
		goto Done;
	Conv16 = (unsigned char*)AllocMemory(ConvW * ConvH * Src->Channel);		//	Save the convolution matrix to remove potentially useless data in the original Conv, and note that there is no 16 byte alignment here.
	if (Conv16 == NULL) {
		Ret = false;
		goto Done;
	}
	Kernel = (unsigned char*)AllocMemory(ConvW * SrcH * Src->Channel);		//	Save the 16 byte aligned convolution kernel matrix to facilitate the use of SSE
	if (Kernel == NULL) {
		Ret = false;
		goto Done;
	}

	for (Y = 0; Y < ConvH; Y++)
		memcpy(Conv16 + Y * ConvW * Src->Channel, Conv->Data + Y * Conv->WidthStep, ConvW * Src->Channel);	//	Replicating the data of the convolution matrix
	for (Y = 0; Y < SrcH; Y++)
		memcpy(Kernel + Y * ConvW * Src->Channel, Src->Data + Y * Src->WidthStep, ConvW * Src->Channel);	//	Calculation of the convolution core data to be sampled in the first column of all pixels

	for (X = 0; X < DestW; X++)
	{
		if (X != 0)													//	If it's not the first column, you need to update the data of the convolution kernel
		{
			memcpy(Kernel, Kernel + Src->Channel, (ConvW * SrcH - 1) * Src->Channel);			//	Move data forward
			LinePS = Src->Data + (X + ConvW - 1) * Src->Channel;
			CurKer = Kernel + (ConvW - 1) * Src->Channel;
			if (Src->Channel == 1)
			{
				for (Y = 0; Y < SrcH; Y++)
				{
					CurKer[0] = LinePS[0];								//	Refresh the next element
					CurKer += ConvW;
					LinePS += Src->WidthStep;
				}
			}
			else
			{
				for (Y = 0; Y < SrcH; Y++)
				{
					CurKer[0] = LinePS[0];								//	Refresh the next element
					CurKer[1] = LinePS[1];
					CurKer[2] = LinePS[2];
					CurKer += ConvW * 3;
					LinePS += Src->WidthStep;
				}
			}
		}

		CurKer = Kernel, LinePD = (int*)(*Dest)->Data + X;
		for (Y = 0; Y < DestH; Y++)										//	Update in the direction of the column
		{
			LinePD[0] = MultiplySSE(Conv16, CurKer, Length);			//	The color images are also just added together.	
			CurKer += ConvW * Src->Channel;
			LinePD += DestW;
		}
	}

Done:
	FreeMemory(Conv16);
	FreeMemory(Kernel);
	return Ret;
}

int GetPowerSum(TMatrix* Src)
{
	if (Src == NULL || Src->Data == NULL) return 0;
	if (Src->Depth != DEPTH_8U) return 0;

	int X, Y, Sum, Width = Src->Width, Height = Src->Height;
	unsigned char* LinePS;

	if (Src->Channel == 1)
	{
		for (Y = 0, Sum = 0; Y < Height; Y++)
		{
			LinePS = Src->Data + Y * Src->WidthStep;
			for (X = 0; X < Width; X++)
			{
				Sum += LinePS[X] * LinePS[X];
			}
		}
	}
	else
	{
		for (Y = 0, Sum = 0; Y < Height; Y++)
		{
			LinePS = Src->Data + Y * Src->WidthStep;
			for (X = 0; X < Width; X++)
			{
				Sum += LinePS[0] * LinePS[0] + LinePS[1] * LinePS[1] + LinePS[2] * LinePS[2];
				LinePS += 3;
			}
		}
	}
	return Sum;
}

/// <summary>
/// 计算图像的累积平方差（速度优化）
/// </summary>
/// <param name="Src">源图像,表示大图 在此图中查找</param>
/// <param name="Template">模板图像，即要找的图片</param>
/// <param name="Dest">结果图像，大小必须为Src->Width-Template->Width+1，Src->Height-Template->Height+1。</param>
/// <remarks> 1:Cumulative mean square variance is the cumulative sum of squares of the difference between two images corresponding to position pixels and: (a-b) ^2 = a^2 + b^2 - 2Ab.</remarks>
/// <remarks> 2:A (template) of the square is a fixed value, B (a small map source shown) square can be used to achieve a fast integral graph, a*b can achieve fast convolution.</remarks>
/// <returns>成功或失败</returns>
bool _declspec(dllexport) _stdcall MatchTemplate(TMatrix* Src, TMatrix* Template, TMatrix** Dest)
{
	if (Src == NULL || Template == NULL)
		return false;
	if (Src->Data == NULL || Template->Data == NULL)
		return false;
	if (Src->Width <= Template->Width || Src->Height <= Template->Height || Src->Channel != Template->Channel || Src->Depth != Template->Depth)
		return false;
	if (Src->Depth != DEPTH_8U || Src->Channel == 4)
		return false;

	int X, Y, Width, Height, PowerSum;
	int* LinePL, * LinePC, * LinePD;
	TMatrix* LocalSquareSum = NULL, * XY = NULL;

	Width = Src->Width - Template->Width + 1, Height = Src->Height - Template->Height + 1;

	bool Ret = CreateMatrix(Width, Height, DEPTH_32S, 1, Dest);								//	Allocation of data
	if (!Ret)
		goto Done;
	Ret = GetLocalSquareSum(Src, &LocalSquareSum, Template->Width, Template->Height);
	if (!Ret)
		goto Done;
	Ret = FastConv2(Src, Template, &XY);
	if (!Ret)
		goto Done;
	PowerSum = GetPowerSum(Template);

	for (Y = 0; Y < Height; Y++)
	{
		LinePL = (int*)(LocalSquareSum->Data + Y * LocalSquareSum->WidthStep);
		LinePC = (int*)(XY->Data + Y * XY->WidthStep);
		LinePD = (int*)((*Dest)->Data + Y * (*Dest)->WidthStep);
		for (X = 0; X < Width; X++)
		{
			LinePD[X] = PowerSum + LinePL[X] - 2 * LinePC[X];					//	a^2 + b^2 - 2ab
		}
	}
Done:
	FreeMatrix(&LocalSquareSum);
	FreeMatrix(&XY);
	return Ret;
}



bool _declspec(dllexport) _stdcall MinMaxLoc(TMatrix* Src, int& Min_PosX, int& Min_PosY, int& Max_PosX, int& Max_PosY)
{
	if (Src == NULL || Src->Data == NULL)
		return false;
	if (Src->Depth != DEPTH_32S || Src->Channel != 1)
		return false;

	int X, Y, Width, Height, Min, Max, Value, * LinePS;

	Width = Src->Width, Height = Src->Height;
	Min = INT_MAX, Max = INT_MIN;

	for (Y = 0; Y < Height; Y++)
	{
		LinePS = (int*)(Src->Data + Y * Src->WidthStep);
		for (X = 0; X < Width; X++)
		{
			Value = LinePS[X];
			if (Min > Value)
			{
				Min = Value;
				Min_PosX = X;
				Min_PosY = Y;
			}
			if (Max < Value)
			{
				Max = Value;
				Max_PosX = X;
				Max_PosY = Y;
			}
		}
	}
Done:
	return true;
}