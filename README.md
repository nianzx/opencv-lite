该项目是因为我嫌弃opencv的库太大，而我只需要图片比对的功能
所以自己搞了一套
在dist里提供已经打包好的dll供c#使用
对外的函数接口原型如下：
```
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
```
TMatrix定义如下：
```
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
```

可以参考这个示例 [opencv-lite-example](https://github.com/nianzx/opencv-lite-example)