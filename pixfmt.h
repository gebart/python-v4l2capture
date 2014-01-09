
#ifndef _PIXFMT_H_
#define _PIXFMT_H_

#include <string>

int DecodeFrame(const unsigned char *data, unsigned dataLen, 
	const char *inPxFmt,
	int &width, int &height,
	const char *targetPxFmt,
	unsigned char **buffOut,
	unsigned *buffOutLen);

int DecodeAndResizeFrame(const unsigned char *data, 
	unsigned dataLen, 
	const char *inPxFmt,
	int srcWidth, int srcHeight,
	const char *targetPxFmt,
	unsigned char **buffOut,
	unsigned *buffOutLen, 
	int dstWidth, 
	int dstHeight);

int ResizeFrame(const unsigned char *data, 
	unsigned dataLen, 
	const char *pxFmt,
	int srcWidth, int srcHeight,
	unsigned char **buffOut,
	unsigned *buffOutLen,
	int dstWidth, 
	int dstHeight);

int ResizeRgb24ImageNN(const unsigned char *data, unsigned dataLen, 
	int widthIn, int heightIn,
	unsigned char *buffOut,
	unsigned buffOutLen,
	int widthOut, int heightOut, int invertVertical = 0, int tupleLen = 3);

int VerticalFlipRgb24(const unsigned char *im, unsigned dataLen, 
	int width, int height,
	unsigned char **buffOut,
	unsigned *buffOutLen);

int InsertHuffmanTableCTypes(const unsigned char* inBufferPtr, unsigned inBufferLen, std::string &outBuffer);

#endif //_PIXFMT_H_

