
#include <stdio.h>
#include <string.h>
#include <jpeglib.h>
#include <setjmp.h>
#include <assert.h>
#include <stdexcept>
#include <iostream>
#include "pixfmt.h"

// *********************************************************************

#define HUFFMAN_SEGMENT_LEN 420

const char huffmanSegment[HUFFMAN_SEGMENT_LEN+1] =
	"\xFF\xC4\x01\xA2\x00\x00\x01\x05\x01\x01\x01\x01"
	"\x01\x01\x00\x00\x00\x00\x00\x00\x00\x00\x01\x02"
	"\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x01\x00\x03"
	"\x01\x01\x01\x01\x01\x01\x01\x01\x01\x00\x00\x00"
	"\x00\x00\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09"
	"\x0A\x0B\x10\x00\x02\x01\x03\x03\x02\x04\x03\x05"
	"\x05\x04\x04\x00\x00\x01\x7D\x01\x02\x03\x00\x04"
	"\x11\x05\x12\x21\x31\x41\x06\x13\x51\x61\x07\x22"
	"\x71\x14\x32\x81\x91\xA1\x08\x23\x42\xB1\xC1\x15"
	"\x52\xD1\xF0\x24\x33\x62\x72\x82\x09\x0A\x16\x17"
	"\x18\x19\x1A\x25\x26\x27\x28\x29\x2A\x34\x35\x36"
	"\x37\x38\x39\x3A\x43\x44\x45\x46\x47\x48\x49\x4A"
	"\x53\x54\x55\x56\x57\x58\x59\x5A\x63\x64\x65\x66"
	"\x67\x68\x69\x6A\x73\x74\x75\x76\x77\x78\x79\x7A"
	"\x83\x84\x85\x86\x87\x88\x89\x8A\x92\x93\x94\x95"
	"\x96\x97\x98\x99\x9A\xA2\xA3\xA4\xA5\xA6\xA7\xA8"
	"\xA9\xAA\xB2\xB3\xB4\xB5\xB6\xB7\xB8\xB9\xBA\xC2"
	"\xC3\xC4\xC5\xC6\xC7\xC8\xC9\xCA\xD2\xD3\xD4\xD5"
	"\xD6\xD7\xD8\xD9\xDA\xE1\xE2\xE3\xE4\xE5\xE6\xE7"
	"\xE8\xE9\xEA\xF1\xF2\xF3\xF4\xF5\xF6\xF7\xF8\xF9"
	"\xFA\x11\x00\x02\x01\x02\x04\x04\x03\x04\x07\x05"
	"\x04\x04\x00\x01\x02\x77\x00\x01\x02\x03\x11\x04"
	"\x05\x21\x31\x06\x12\x41\x51\x07\x61\x71\x13\x22"
	"\x32\x81\x08\x14\x42\x91\xA1\xB1\xC1\x09\x23\x33"
	"\x52\xF0\x15\x62\x72\xD1\x0A\x16\x24\x34\xE1\x25"
	"\xF1\x17\x18\x19\x1A\x26\x27\x28\x29\x2A\x35\x36"
	"\x37\x38\x39\x3A\x43\x44\x45\x46\x47\x48\x49\x4A"
	"\x53\x54\x55\x56\x57\x58\x59\x5A\x63\x64\x65\x66"
	"\x67\x68\x69\x6A\x73\x74\x75\x76\x77\x78\x79\x7A"
	"\x82\x83\x84\x85\x86\x87\x88\x89\x8A\x92\x93\x94"
	"\x95\x96\x97\x98\x99\x9A\xA2\xA3\xA4\xA5\xA6\xA7"
	"\xA8\xA9\xAA\xB2\xB3\xB4\xB5\xB6\xB7\xB8\xB9\xBA"
	"\xC2\xC3\xC4\xC5\xC6\xC7\xC8\xC9\xCA\xD2\xD3\xD4"
	"\xD5\xD6\xD7\xD8\xD9\xDA\xE2\xE3\xE4\xE5\xE6\xE7"
	"\xE8\xE9\xEA\xF2\xF3\xF4\xF5\xF6\xF7\xF8\xF9\xFA";

int ReadJpegFrame(const unsigned char *data, unsigned offset, const unsigned char **twoBytesOut, unsigned *frameStartPosOut, unsigned *cursorOut)
{
	//Based on http://www.gdcl.co.uk/2013/05/02/Motion-JPEG.html
	//and https://en.wikipedia.org/wiki/JPEG

	*twoBytesOut = NULL;
	*frameStartPosOut = 0;
	*cursorOut = 0;
	unsigned cursor = offset;
	//Check frame start
	unsigned frameStartPos = offset;
	const unsigned char *twoBytes = &data[cursor];

	if (twoBytes[0] != 0xff)
	{
		//print "Error: found header", map(hex,twoBytes),"at position",cursor
		return 0;
	}

	cursor = 2 + cursor;

	//Handle padding
	int paddingByte = (twoBytes[0] == 0xff && twoBytes[1] == 0xff);
	if(paddingByte)
	{
		*twoBytesOut = twoBytes;
		*frameStartPosOut = frameStartPos;
		*cursorOut = cursor;
		return 1;
	}

	//Structure markers with 2 byte length
	int markHeader = (twoBytes[0] == 0xff && twoBytes[1] >= 0xd0 && twoBytes[1] <= 0xd9);
	if (markHeader)
	{
		*twoBytesOut = twoBytes;
		*frameStartPosOut = frameStartPos;
		*cursorOut = cursor;
		return 1;
	}

	//Determine length of compressed (entropy) data
	int compressedDataStart = (twoBytes[0] == 0xff && twoBytes[1] == 0xda);
	if (compressedDataStart)
	{
		unsigned sosLength = ((data[cursor] << 8) + data[cursor+1]);
		cursor += sosLength;

		//Seek through frame
		int run = 1;
		while(run)
		{
			unsigned char byte = data[cursor];
			cursor += 1;
			
			if(byte == 0xff)
			{
				unsigned char byte2 = data[cursor];
				cursor += 1;
				if(byte2 != 0x00)
				{
					if(byte2 >= 0xd0 && byte2 <= 0xd8)
					{
						//Found restart structure
						//print hex(byte), hex(byte2)
					}
					else
					{
						//End of frame
						run = 0;
						cursor -= 2;
					}
				}
				else
				{
					//Add escaped 0xff value in entropy data
				}	
			}
			else
			{
				
			}
		}

		*twoBytesOut = twoBytes;
		*frameStartPosOut = frameStartPos;
		*cursorOut = cursor;
		return 1;
	}

	//More cursor for all other segment types
	unsigned segLength = (data[cursor] << 8) + data[cursor+1];
	cursor += segLength;
	*twoBytesOut = twoBytes;
	*frameStartPosOut = frameStartPos;
	*cursorOut = cursor;
	return 1;
}

int InsertHuffmanTableCTypes(const unsigned char* inBufferPtr, unsigned inBufferLen, std::string &outBuffer)
{
	int parsing = 1;
	unsigned frameStartPos = 0;
	int huffFound = 0;

	outBuffer.clear();

	while(parsing)
	{
		//Check if we should stop
		if (frameStartPos >= inBufferLen)
		{
			parsing = 0;
			continue;
		}

		//Read the next segment
		const unsigned char *twoBytes = NULL;
		unsigned frameEndPos=0;
	
		int ok = ReadJpegFrame(inBufferPtr, frameStartPos, &twoBytes, &frameStartPos, &frameEndPos);

		//if(verbose)
		//	print map(hex, twoBytes), frameStartPos, frameEndPos;

		//Stop if there is a serious error
		if(!ok)
		{
			return 0;
		}
	
		//Check if this segment is the compressed data
		if(twoBytes[0] == 0xff && twoBytes[1] == 0xda && !huffFound)
		{
			outBuffer.append(huffmanSegment, HUFFMAN_SEGMENT_LEN);
		}

		//Check the type of frame
		if(twoBytes[0] == 0xff && twoBytes[1] == 0xc4)
			huffFound = 1;

		//Write current structure to output
		outBuffer.append((char *)&inBufferPtr[frameStartPos], frameEndPos - frameStartPos);

		//Move cursor
		frameStartPos = frameEndPos;
	}
	return 1;
}

// *********************************************************************

struct my_error_mgr
{
	struct jpeg_error_mgr pub;	/* "public" fields */

	jmp_buf setjmp_buffer;	/* for return to caller */
};

typedef struct my_error_mgr * my_error_ptr;
METHODDEF(void) my_error_exit (j_common_ptr cinfo)
{
	my_error_ptr myerr = (my_error_ptr) cinfo->err;	 

	/* Always display the message. */
	(*cinfo->err->output_message) (cinfo);

	/* Return control to the setjmp point */
	longjmp(myerr->setjmp_buffer, 1);
}

int ReadJpegFile(unsigned char * inbuffer,
	unsigned long insize,
	unsigned char **outBuffer,
	unsigned *outBufferSize,
	int *widthOut, int *heightOut, int *channelsOut)
{
	/* This struct contains the JPEG decompression parameters and pointers to
	 * working space (which is allocated as needed by the JPEG library).
	 */

	if(inbuffer[0] != 0xFF || inbuffer[1] != 0xD8)
		return 0;

	struct jpeg_decompress_struct cinfo;
	struct my_error_mgr jerr;
	*outBuffer = NULL;
	*outBufferSize = 0;
	*widthOut = 0;
	*heightOut = 0;
	*channelsOut = 0;

	/* More stuff */
	JSAMPARRAY buffer;		/* Output row buffer */
	int row_stride;		/* physical row width in output buffer */

	/* Step 1: initialize the JPEG decompression object. */
	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = my_error_exit;
	/* Establish the setjmp return context for my_error_exit to use. */
	if (setjmp(jerr.setjmp_buffer)) {
		/* If we get here, the JPEG code has signaled an error.
		 * We need to clean up the JPEG object, close the input file, and return.
		 */
		jpeg_destroy_decompress(&cinfo);
		return 0;
	}

	jpeg_create_decompress(&cinfo);

	/* Step 2: specify data source */
	jpeg_mem_src(&cinfo, inbuffer, insize);

	/* Step 3: read file parameters with jpeg_read_header() */
	jpeg_read_header(&cinfo, TRUE);

	unsigned int outBuffLen = cinfo.image_width * cinfo.image_height * cinfo.num_components;
	if(*outBufferSize != 0 && *outBufferSize != outBuffLen)
		throw std::runtime_error("Output buffer has incorrect size");
	if(*outBuffer == NULL)
	{
		*outBuffer = new unsigned char[*outBufferSize];
	}
	*outBufferSize = outBuffLen;
	*widthOut = cinfo.image_width;
	*heightOut = cinfo.image_height;
	*channelsOut = cinfo.num_components;

	/* Step 4: set parameters for decompression */
	//Optional

	/* Step 5: Start decompressor */
	jpeg_start_decompress(&cinfo);
	/* JSAMPLEs per row in output buffer */
	row_stride = cinfo.output_width * cinfo.output_components;
	/* Make a one-row-high sample array that will go away when done with image */
	buffer = (*cinfo.mem->alloc_sarray)
		((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);

	/* Step 6: while (scan lines remain to be read) */
	/*					 jpeg_read_scanlines(...); */

	/* Here we use the library's state variable cinfo.output_scanline as the
	 * loop counter, so that we don't have to keep track ourselves.
	 */
	while (cinfo.output_scanline < cinfo.output_height) {
		/* jpeg_read_scanlines expects an array of pointers to scanlines.
		 * Here the array is only one element long, but you could ask for
		 * more than one scanline at a time if that's more convenient.
		 */
		jpeg_read_scanlines(&cinfo, buffer, 1);
		/* Assume put_scanline_someplace wants a pointer and sample count. */
		//put_scanline_someplace(buffer[0], row_stride);
		assert(row_stride = cinfo.image_width * cinfo.num_components);
		//printf("%ld\n", (long)buffer);
		//printf("%ld\n", (long)buffer[0]);
		//printf("%d %d\n", (cinfo.output_scanline-1) * row_stride, *outBufferSize);
		//printf("%ld %ld\n", (long)outBuffer, (long)&outBuffer[(cinfo.output_scanline-1) * row_stride]);
		memcpy(&(*outBuffer)[(cinfo.output_scanline-1) * row_stride], buffer[0], row_stride);
	}

	/* Step 7: Finish decompression */
	jpeg_finish_decompress(&cinfo);

	/* Step 8: Release JPEG decompression object */

	/* This is an important step since it will release a good deal of memory. */
	jpeg_destroy_decompress(&cinfo);

	/* At this point you may want to check to see whether any corrupt-data
	 * warnings occurred (test whether jerr.pub.num_warnings is nonzero).
	 */

	return 1;
}

// **************************************************************

int ConvertRGBtoYUYVorSimilar(const unsigned char *im, unsigned sizeimage, 
	unsigned width, unsigned height, const char *targetPxFmt,
	unsigned char **outIm, unsigned *outImSize)
{
	unsigned bytesperline = width * 2;
	unsigned padding = 0;
	if(*outImSize != 0 && *outImSize != sizeimage)
		throw std::runtime_error("Output buffer has incorrect size");
	unsigned char *outBuff = *outIm;
	if(*outIm == NULL)
	{
		outBuff = new unsigned char [*outImSize];
		*outIm = outBuff;
	}
	*outImSize = sizeimage+padding;
	unsigned char *im2 = (unsigned char *)im;

	int uOffset = 0;
	int vOffset = 0;
	int yOffset1 = 0;
	int yOffset2 = 0;
	int formatKnown = 0;

	if(strcmp(targetPxFmt, "YUYV")==0)
	{
		uOffset = 1;
		vOffset = 3;
		yOffset1 = 0;
		yOffset2 = 2;
		formatKnown = 1;
	}

	if(strcmp(targetPxFmt, "UYVY")==0)
	{
		uOffset = 0;
		vOffset = 2;
		yOffset1 = 1;
		yOffset2 = 3;
		formatKnown = 1;
	}

	if(!formatKnown)
	{
		throw std::runtime_error("Unknown target pixel format");
	}

	for (unsigned y=0; y<height; y++)
	{
		//Set lumenance
		unsigned cursor = y * bytesperline + padding;
		for(unsigned x=0;x< width;x+=2)
		{
			unsigned rgbOffset = width * y * 3 + x * 3;
			outBuff[cursor+yOffset1] = (unsigned char)(im[rgbOffset] * 0.299 + im[rgbOffset+1] * 0.587 + im[rgbOffset+2] * 0.114 + 0.5);
			outBuff[cursor+yOffset2] = (unsigned char)(im[rgbOffset+3] * 0.299 + im[rgbOffset+4] * 0.587 + im[rgbOffset+5] * 0.114 + 0.5);
			cursor += 4;
		}
	
		//Set color information for Cb
		cursor = y * bytesperline + padding;
		for(unsigned x=0;x< width;x+=2)
		{
			unsigned rgbOffset = width * y * 3 + x * 3;
			double Pb1 = im2[rgbOffset+0] * -0.168736 + im2[rgbOffset+1] * -0.331264 + im2[rgbOffset+2] * 0.5;
			double Pb2 = im2[rgbOffset+3] * -0.168736 + im2[rgbOffset+4] * -0.331264 + im2[rgbOffset+5] * 0.5;

			outBuff[cursor+uOffset] = (unsigned char)(0.5 * (Pb1 + Pb2) + 128.5);
			cursor += 4;
		}

		//Set color information for Cr
		cursor = y * bytesperline + padding;
		for(unsigned x=0;x< width;x+=2)
		{
			unsigned rgbOffset = width * y * 3 + x * 3;
			double Pr1 = im2[rgbOffset+0] * 0.5 + im2[rgbOffset+1] * -0.418688 + im2[rgbOffset+2] * -0.081312;
			double Pr2 = im2[rgbOffset+3] * 0.5 + im2[rgbOffset+4] * -0.418688 + im2[rgbOffset+5] * -0.081312;

			outBuff[cursor+vOffset] = (unsigned char)(0.5 * (Pr1 + Pr2) + 128.5);
			cursor += 4;
		}
	}

	return 1;
}

// *********************************************************************

int ConvertRgb24ToI420orYV12(const unsigned char *im, unsigned dataLen, 
	int width, int height,
	unsigned char **buffOut,
	unsigned *buffOutLen,
	const char *outPxFmt)
{
	//Check if input buffer is of sufficient size
	int requiredInputSize = 3 * width * height;
	if(requiredInputSize != dataLen)
		throw std::runtime_error("Input buffer has unexpected size");

	//Create output buffer if required
	int requiredSize = width * height * 1.5;
	if(*buffOutLen != 0 && *buffOutLen != requiredSize)
		throw std::runtime_error("Output buffer has incorrect size");
	*buffOutLen = requiredSize;
	if(*buffOut == NULL)
		*buffOut = new unsigned char [*buffOutLen];

	//memset(*buffOut, 128, *buffOutLen);
	unsigned uPlaneOffset = 0;
	unsigned vPlaneOffset = 0;

	if(strcmp(outPxFmt, "I420")==0)
	{
		uPlaneOffset = width * height;
		vPlaneOffset = width * height * 1.25;
	}
	else
	{
		//Assume YV12
		uPlaneOffset = width * height * 1.25;
		vPlaneOffset = width * height;
	}

	for(int x = 0; x < width ; x+=2)
	{
		for(int y = 0; y < height; y+=2)
		{
			unsigned YOutOffset1 = width * y + x;
			unsigned rgbInOffset1 = width * y * 3 + x * 3;
			unsigned YOutOffset2 = width * y + (x+1);
			unsigned rgbInOffset2 = width * y * 3 + (x+1) * 3;
			unsigned YOutOffset3 = width * (y+1) + x;
			unsigned rgbInOffset3 = width * (y+1) * 3 + x * 3;
			unsigned YOutOffset4 = width * (y+1) + (x+1);
			unsigned rgbInOffset4 = width * (y+1) * 3 + (x+1) * 3;

			unsigned colOffset = (width/2) * (y/2) + (x/2);
			unsigned UOutOffset = colOffset + uPlaneOffset;
			unsigned VOutOffset = colOffset + vPlaneOffset;

			if(rgbInOffset1+2 >= dataLen) {throw std::runtime_error("1");}
			if(rgbInOffset2+2 >= dataLen) {throw std::runtime_error("2");}
			if(rgbInOffset3+2 >= dataLen) {throw std::runtime_error("3");}
			if(rgbInOffset4+2 >= dataLen) {throw std::runtime_error("4");}

			unsigned Y1 = 66 * im[rgbInOffset1] + 129 * im[rgbInOffset1+1] + 25 * im[rgbInOffset1+2];
			unsigned Y2 = 66 * im[rgbInOffset2] + 129 * im[rgbInOffset2+1] + 25 * im[rgbInOffset2+2];
			unsigned Y3 = 66 * im[rgbInOffset3] + 129 * im[rgbInOffset3+1] + 25 * im[rgbInOffset3+2];
			unsigned Y4 = 66 * im[rgbInOffset4] + 129 * im[rgbInOffset4+1] + 25 * im[rgbInOffset4+2];

			unsigned U1 = -38 * im[rgbInOffset1] - 74 * im[rgbInOffset1+1] + 112 * im[rgbInOffset1+2];
			unsigned U2 = -38 * im[rgbInOffset2] - 74 * im[rgbInOffset2+1] + 112 * im[rgbInOffset2+2];
			unsigned U3 = -38 * im[rgbInOffset3] - 74 * im[rgbInOffset3+1] + 112 * im[rgbInOffset3+2];
			unsigned U4 = -38 * im[rgbInOffset4] - 74 * im[rgbInOffset4+1] + 112 * im[rgbInOffset4+2];

			unsigned V1 = 112 * im[rgbInOffset1] - 94 * im[rgbInOffset1+1] - 18 * im[rgbInOffset1+2];
			unsigned V2 = 112 * im[rgbInOffset2] - 94 * im[rgbInOffset2+1] - 18 * im[rgbInOffset2+2];
			unsigned V3 = 112 * im[rgbInOffset3] - 94 * im[rgbInOffset3+1] - 18 * im[rgbInOffset3+2];
			unsigned V4 = 112 * im[rgbInOffset4] - 94 * im[rgbInOffset4+1] - 18 * im[rgbInOffset4+2];

			Y1 = ((Y1 + 128) >> 8) + 16;
			Y2 = ((Y2 + 128) >> 8) + 16;
			Y3 = ((Y3 + 128) >> 8) + 16;
			Y4 = ((Y4 + 128) >> 8) + 16;

			U1 = ((U1 + 128) >> 8) + 128;
			U2 = ((U2 + 128) >> 8) + 128;
			U3 = ((U3 + 128) >> 8) + 128;
			U4 = ((U4 + 128) >> 8) + 128;

			V1 = ((V1 + 128) >> 8) + 128;
			V2 = ((V2 + 128) >> 8) + 128;
			V3 = ((V3 + 128) >> 8) + 128;
			V4 = ((V4 + 128) >> 8) + 128;

			if(YOutOffset1 >= *buffOutLen) {throw std::runtime_error("5");}
			if(YOutOffset2 >= *buffOutLen) {throw std::runtime_error("6");}
			if(YOutOffset3 >= *buffOutLen) {throw std::runtime_error("7");}
			if(YOutOffset4 >= *buffOutLen) {throw std::runtime_error("8");}
			if(VOutOffset >= *buffOutLen) {throw std::runtime_error("9");}
			if(UOutOffset >= *buffOutLen) {throw std::runtime_error("10");}

			(*buffOut)[YOutOffset1] = Y1;
			(*buffOut)[YOutOffset2] = Y2;
			(*buffOut)[YOutOffset3] = Y3;
			(*buffOut)[YOutOffset4] = Y4;

			(*buffOut)[VOutOffset] = (unsigned char)((V1+V2+V3+V4)/4.+0.5);
			(*buffOut)[UOutOffset] = (unsigned char)((U1+U2+U3+U4)/4.+0.5);
		}
	}
	
	return 1;
}

int ConvertYUYVtoRGB(const unsigned char *im, unsigned dataLen, 
	int width, int height,
	unsigned char **buffOut,
	unsigned *buffOutLen)
{
	// Convert buffer from YUYV to RGB.
	// For the byte order, see: http://v4l2spec.bytesex.org/spec/r4339.htm
	// For the color conversion, see: http://v4l2spec.bytesex.org/spec/x2123.htm
	unsigned int outBuffLen = dataLen * 6 / 4;
	if(*buffOutLen != 0 && *buffOutLen != outBuffLen)
		throw std::runtime_error("Output buffer has incorrect length");
	*buffOutLen = outBuffLen;
	char *rgb = (char*)*buffOut;
	if(*buffOut == NULL)
	{
		rgb = new char[*buffOutLen];
		*buffOut = (unsigned char*)rgb;
	}

	char *rgb_max = rgb + *buffOutLen;
	const unsigned char *yuyv = im;

#define CLAMP(c) ((c) <= 0 ? 0 : (c) >= 65025 ? 255 : (c) >> 8)
	while(rgb < rgb_max)
		{
			int u = yuyv[1] - 128;
			int v = yuyv[3] - 128;
			int uv = 100 * u + 208 * v;
			u *= 516;
			v *= 409;

			int y = 298 * (yuyv[0] - 16);
			rgb[0] = CLAMP(y + v);
			rgb[1] = CLAMP(y - uv);
			rgb[2] = CLAMP(y + u);

			y = 298 * (yuyv[2] - 16);
			rgb[3] = CLAMP(y + v);
			rgb[4] = CLAMP(y - uv);
			rgb[5] = CLAMP(y + u);

			rgb += 6;
			yuyv += 4;
		}
#undef CLAMP
	return 1;
}

// *********************************************************************

int DecodeFrame(const unsigned char *data, unsigned dataLen, 
	const char *inPxFmt,
	int width, int height,
	const char *targetPxFmt,
	unsigned char **buffOut,
	unsigned *buffOutLen)
{
	//Check if input format and output format match
	if(strcmp(inPxFmt, targetPxFmt) == 0)
	{
		//Conversion not required, return a copy
		if (*buffOutLen != 0 && *buffOutLen != dataLen)
		{
			throw std::runtime_error("Output buffer has incorrect size");
		}
		if(*buffOut == NULL)
		{
			*buffOut = new unsigned char[dataLen];
		}
		*buffOutLen = dataLen;
		memcpy(*buffOut, data, dataLen);
		return 1;
	}

	//MJPEG frame to RGB24
	if(strcmp(inPxFmt,"MJPEG")==0 && strcmp(targetPxFmt, "RGB24")==0)
	{
		std::string jpegBin;
		InsertHuffmanTableCTypes(data, dataLen, jpegBin);

		unsigned char *decodedBuff = NULL;
		unsigned decodedBuffSize = 0;
		int widthActual = 0, heightActual = 0, channelsActual = 0;

		int jpegOk = ReadJpegFile((unsigned char*)jpegBin.c_str(), jpegBin.length(), 
			&decodedBuff, 
			&decodedBuffSize, 
			&widthActual, &heightActual, &channelsActual);

		if (!jpegOk)
			throw std::runtime_error("Error decoding jpeg");

		if(widthActual == width && heightActual == height)
		{
			assert(channelsActual == 3);
			*buffOut = decodedBuff;
			*buffOutLen = decodedBuffSize;
		}
		else
		{
			delete [] decodedBuff;
			throw std::runtime_error("Decoded jpeg has unexpected size");
		}
		return 1;
	}

	//YUYV to RGB24
	if(strcmp(inPxFmt,"YUYV")==0 && strcmp(targetPxFmt, "RGB24")==0)
	{
		int ret = ConvertYUYVtoRGB(data, dataLen, 
			width, height,
			buffOut,
			buffOutLen);
		return ret;
	}

	//RGB24 to I420 or YV12
	if(strcmp(inPxFmt,"RGB24")==0 && 
		(strcmp(targetPxFmt, "I420")==0 || strcmp(targetPxFmt, "YV12")==0))
	{
		int ret = ConvertRgb24ToI420orYV12(data, dataLen, 
			width, height,
			buffOut, buffOutLen, targetPxFmt);
		return ret;
	}

	//RGB24 to YUYV or UYVY
	if(strcmp(inPxFmt,"RGB24")==0 && 
		(strcmp(targetPxFmt, "YUYV")==0
		|| strcmp(targetPxFmt, "UYVY")==0)
		)
	{
		int ret = ConvertRGBtoYUYVorSimilar(data, dataLen, 
			width, height, targetPxFmt,
			buffOut, buffOutLen);
		return ret;
	}

	//RGB24 -> BGR24
	if(strcmp(inPxFmt,"RGB24")==0 && strcmp(targetPxFmt, "BGR24")==0)
	{
		if(*buffOutLen != 0 && *buffOutLen != dataLen)
			throw std::runtime_error("Output buffer has incorrect size");
		if(*buffOut == NULL)
			*buffOut = new unsigned char[dataLen];
		*buffOutLen = dataLen;
		for(unsigned i = 0; i+2 < dataLen; i+=3)
		{
			(*buffOut)[i+0] = data[i+2];
			(*buffOut)[i+1] = data[i+1];
			(*buffOut)[i+2] = data[i+0];
		}
		return 1;
	}

	//If no direct conversion to BGR24 is possible, convert to RGB24
	//as an intermediate step
	if(strcmp(targetPxFmt, "BGR24")==0)
	{
		unsigned char *rgbBuff = NULL;
		unsigned rgbBuffLen = 0;
		int ret = DecodeFrame(data, dataLen, 
			inPxFmt,
			width, height,
			"RGB24",
			&rgbBuff,
			&rgbBuffLen);

		if(ret>0)
		{
			int ret2 = DecodeFrame(rgbBuff, rgbBuffLen,
				"RBG24",
				width, height,
				targetPxFmt,
				buffOut,
				buffOutLen);
			delete [] rgbBuff;
			if(ret2>0) return ret2;
		}
	}

	//Destination of RGB24INV, so convert to RGB24 first
	if(strcmp(targetPxFmt, "RGB24INV")==0)
	{
		unsigned char *rgbBuff = NULL;
		unsigned rgbBuffLen = 0;
		int ret = DecodeFrame(data, dataLen, 
			inPxFmt,
			width, height,
			"RGB24",
			&rgbBuff,
			&rgbBuffLen);

		if(ret>0)
		{
			int ret2 = VerticalFlipRgb24(rgbBuff, rgbBuffLen, 
				width, height,
				buffOut,
				buffOutLen);
			delete [] rgbBuff;
			if(ret2>0) return ret2;
		}
	}

	//Vertical flip of RGB24
	if(strcmp(inPxFmt, "RGB24INV")==0)
	{
		unsigned char *rgbBuff = NULL;
		unsigned rgbBuffLen = 0;
		int ret = VerticalFlipRgb24(data, dataLen, 
				width, height,
				&rgbBuff,
				&rgbBuffLen);

		if(ret>0)
		{
			int ret2 = DecodeFrame(rgbBuff, rgbBuffLen,
				"RGB24",
				width, height,
				targetPxFmt,
				buffOut,
				buffOutLen);
			delete [] rgbBuff;
			if(ret2>0) return ret2;
		}
	}

	/*
	//Untested code
	if((strcmp(inPxFmt,"YUV2")==0 || strcmp(inPxFmt,"YVU2")==0) 
		&& strcmp(targetPxFmt, "RGB24")==0)
	{
		int uoff = 1;
		int voff = 3;
		if(strcmp(inPxFmt,"YUV2")==0)
		{
			uoff = 1;
			voff = 3;
		}
		if(strcmp(inPxFmt,"YVU2")==0)
		{
			uoff = 3;
			voff = 1;
		}

		int stride = width * 4;
		int hwidth = width/2;
		for(int lineNum=0; lineNum < height; lineNum++)
		{
			int lineOffset = lineNum * stride;
			int outOffset = lineNum * width * 3;
			
			for(int pxPairNum=0; pxPairNum < hwidth; pxPairNum++)
			{
			unsigned char Y1 = data[pxPairNum * 4 + lineOffset];
			unsigned char Cb = data[pxPairNum * 4 + lineOffset + uoff];
			unsigned char Y2 = data[pxPairNum * 4 + lineOffset + 2];
			unsigned char Cr = data[pxPairNum * 4 + lineOffset + voff];

			//ITU-R BT.601 colour conversion
			double R1 = (Y1 + 1.402 * (Cr - 128));
			double G1 = (Y1 - 0.344 * (Cb - 128) - 0.714 * (Cr - 128));
			double B1 = (Y1 + 1.772 * (Cb - 128));
			double R2 = (Y2 + 1.402 * (Cr - 128));
			double G2 = (Y2 - 0.344 * (Cb - 128) - 0.714 * (Cr - 128));
			double B2 = (Y2 + 1.772 * (Cb - 128));

			(*buffOut)[outOffset + pxPairNum * 6] = R1;
			(*buffOut)[outOffset + pxPairNum * 6 + 1] = G1;
			(*buffOut)[outOffset + pxPairNum * 6 + 2] = B1;
			(*buffOut)[outOffset + pxPairNum * 6 + 3] = R2;
			(*buffOut)[outOffset + pxPairNum * 6 + 4] = G2;
			(*buffOut)[outOffset + pxPairNum * 6 + 5] = B2;
			}
		}
	}
	*/

	return 0;
}

// ************* Resize Code *******************************

int ResizeRgb24ImageNN(const unsigned char *data, unsigned dataLen, 
	int widthIn, int heightIn,
	unsigned char *buffOut,
	unsigned buffOutLen,
	int widthOut, int heightOut, int invertVertical, int tupleLen)
{
	//Simple crop of image to target buffer
	for(int x = 0; x < widthOut; x++)
	{
		for(int y = 0; y < heightOut; y++)
		{
			unsigned outOffset = x*tupleLen + (y*tupleLen*widthOut);
			if(outOffset + tupleLen >= buffOutLen) continue;
			unsigned char *outPx = &buffOut[outOffset];

			//Scale position
			double inx = (double)x * (double)widthIn / (double)widthOut;
			double iny = (double)y * (double)heightIn / (double)heightOut;

			//Round to nearest pixel
			int inxi = (int)(inx+0.5);
			int inyi = (int)(iny+0.5);

			int row = inyi;
			if(invertVertical) row = heightIn - inyi - 1;
			unsigned inOffset = inxi*tupleLen + (row*tupleLen*widthIn);
			if(inOffset + tupleLen >= dataLen) continue;
			const unsigned char *inPx = &data[inOffset];

			for(int c = 0; c < tupleLen; c++)
				outPx[c] = inPx[c];
		}

	}

	return 1;
}

int CropToFitRgb24Image(const unsigned char *data, unsigned dataLen, 
	int widthIn, int heightIn,
	unsigned char *buffOut,
	unsigned buffOutLen,
	int widthOut, int heightOut, int invertVertical, int tupleLen = 3)
{
	//Simple crop of image to target buffer
	for(int x = 0; x < widthOut; x++)
	{
		for(int y = 0; y < heightOut; y++)
		{
			unsigned outOffset = x*tupleLen + (y*tupleLen*widthOut);
			if(outOffset + tupleLen >= buffOutLen) continue;
			unsigned char *outPx = &buffOut[outOffset];

			int row = y;
			if(invertVertical) row = heightIn - y - 1;
			unsigned inOffset = x*tupleLen + (row*tupleLen*widthIn);
			if(inOffset + tupleLen >= dataLen) continue;
			const unsigned char *inPx = &data[inOffset];

			for(int c = 0; c < tupleLen; c++)
				outPx[c] = inPx[c];
		}
	}

	return 1;
}

int ResizeFrame(const unsigned char *data, 
	unsigned dataLen, 
	const char *pxFmt,
	int srcWidth, int srcHeight,
	unsigned char **buffOut,
	unsigned *buffOutLen,
	int dstWidth, 
	int dstHeight)
{
	if(strcmp(pxFmt,"RGB24")==0 || strcmp(pxFmt,"BGR24")==0)
	{
		//Allocate new buffer if needed
		int dstBuffSize = 3 * dstWidth * dstHeight;
		if(*buffOutLen != 0 && *buffOutLen != dstBuffSize)
			throw std::runtime_error("Output buffer has incorrect size");
		*buffOutLen = dstBuffSize;
		if(*buffOut == NULL)
			*buffOut = new unsigned char [*buffOutLen];

		return ResizeRgb24ImageNN(data, dataLen, 
			srcWidth, srcHeight,
			*buffOut,
			*buffOutLen,
			dstWidth, dstHeight, 0, 3);
	}
	//Not supported
	return 0;
}

/// **************************************************************

int VerticalFlipRgb24(const unsigned char *im, unsigned dataLen, 
	int width, int height,
	unsigned char **buffOut,
	unsigned *buffOutLen)
{
	//RGB24 -> RGB24INV
	//RGB24INV -> RGB24

	if(dataLen != width * height * 3)
		throw std::runtime_error("Input buffer has incorrect size");
	if(*buffOutLen != 0 && *buffOutLen != dataLen)
		throw std::runtime_error("Output buffer has incorrect size");
	if(*buffOut == NULL)
		*buffOut = new unsigned char[dataLen];
	*buffOutLen = dataLen;

	for(int y = 0; y < height; y++)
	{
		int invy = height - y - 1;
		const unsigned char *inRow = &im[y * width * 3];
		unsigned char *outRow = &((*buffOut)[invy * width * 3]);
		memcpy(outRow, inRow, width * 3);
	}
	return 1;
}

// ****** Combined resize and convert *************************************************


int DecodeAndResizeFrame(const unsigned char *data, 
	unsigned dataLen, 
	const char *inPxFmt,
	int srcWidth, int srcHeight,
	const char *targetPxFmt,
	unsigned char **buffOut,
	unsigned *buffOutLen, 
	int dstWidth, 
	int dstHeight)
{
	if(srcWidth==dstWidth && srcHeight==dstHeight)
	{
		//Resize is not required
		int ret = DecodeFrame(data, dataLen, 
			inPxFmt,
			srcWidth, srcHeight,
			targetPxFmt,
			buffOut,
			buffOutLen);
		return ret;
	}

	const unsigned char *currentImg = data;
	unsigned currentLen = dataLen;
	std::string currentPxFmt = inPxFmt;
	int currentWidth = srcWidth;
	int currentHeight = srcHeight;

	unsigned char *tmpBuff = NULL;
	unsigned tmpBuffLen = 0;
	int resizeRet = ResizeFrame(currentImg, 
		currentLen, 
		currentPxFmt.c_str(),
		currentWidth, currentHeight,
		&tmpBuff,
		&tmpBuffLen,
		dstWidth, 
		dstHeight);

	if(resizeRet > 0)
	{
		//Resize succeeded
		currentImg = tmpBuff;
		currentLen = tmpBuffLen;
		currentWidth = dstWidth;
		currentHeight = dstHeight;

		int decodeRet = DecodeFrame(currentImg, currentLen, 
			currentPxFmt.c_str(),
			currentWidth, currentHeight,
			targetPxFmt,
			buffOut,
			buffOutLen);

		//Free intermediate buff
		if(tmpBuff != NULL)
		{
			delete [] tmpBuff;
			tmpBuff = NULL;
		}

		return decodeRet;
	}

	//Attempt to convert pixel format first, do resize later
	tmpBuff = NULL;
	tmpBuffLen = 0;
	int decodeRet = DecodeFrame(data, dataLen, 
		inPxFmt,
		srcWidth, srcHeight,
		targetPxFmt,
		&tmpBuff,
		&tmpBuffLen);

	if(decodeRet <= 0)
		return 0; //Conversion failed

	//Now resize
	resizeRet = ResizeFrame(tmpBuff, 
		tmpBuffLen, 
		targetPxFmt,
		srcWidth, srcHeight,
		buffOut,
		buffOutLen,
		dstWidth, 
		dstHeight);

	//Free intermediate buff
	if(tmpBuff != NULL)
	{
		delete [] tmpBuff;
		tmpBuff = NULL;
	}

	return resizeRet;
}
