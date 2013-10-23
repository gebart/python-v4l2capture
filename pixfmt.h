
#ifndef _PIXFMT_H_
#define _PIXFMT_H_

#include <string>

int DecodeFrame(const unsigned char *data, unsigned dataLen, 
	const char *inPxFmt,
	int width, int height,
	const char *targetPxFmt,
	unsigned char **buffOut,
	unsigned *buffOutLen);

int InsertHuffmanTableCTypes(const unsigned char* inBufferPtr, unsigned inBufferLen, std::string &outBuffer);

#endif //_PIXFMT_H_

