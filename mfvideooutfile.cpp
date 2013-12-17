
#include "mfvideooutfile.h"
#include "pixfmt.h"
#include <iostream>
#include <stdexcept>
#include <mfapi.h>
#include <Mferror.h>
using namespace std;

template <class T> void SafeRelease(T **ppT)
{
	if (*ppT)
	{
		(*ppT)->Release();
		*ppT = NULL;
	}
}

std::wstring CStringToWString(const char *inStr)
{
	wchar_t *tmpDevName = new wchar_t[strlen(inStr)+1];
	size_t returnValue;
	
	mbstowcs_s(&returnValue, tmpDevName, strlen(inStr)+1, inStr, strlen(inStr)+1);
	std::wstring tmpDevName2(tmpDevName);
	delete [] tmpDevName;
	return tmpDevName2;
}

const UINT32 VIDEO_FPS = 25;
const UINT32 BYTES_PER_TUPLE = 3;

MfVideoOutFile::MfVideoOutFile(const char *fiName) : Base_Video_Out()
{
	HRESULT hr = MFStartup(MF_VERSION);
	if(!SUCCEEDED(hr))
		throw std::runtime_error("Media foundation startup failed");

	hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	if(hr == RPC_E_CHANGED_MODE)
		throw std::runtime_error("CoInitializeEx failed");

	this->pSinkWriter = NULL;
	this->streamIndex = 0;
	this->rtStart = 0;
	this->pxFmt = "BGR24";

	this->outputWidth = 640;
	this->outputHeight = 480;
	this->bitRate = 800000;
	this->fina = CStringToWString(fiName);
}

MfVideoOutFile::~MfVideoOutFile()
{
	this->CloseFile();

	MFShutdown();

	CoUninitialize();
}

void MfVideoOutFile::OpenFile()
{

	if(this->pSinkWriter != NULL)
	{
		throw std::runtime_error("Video output file already open");
	}
	this->rtStart = 0;
	IMFMediaType	*pMediaTypeOut = NULL;   
	IMFMediaType	*pMediaTypeIn = NULL;
	MFFrameRateToAverageTimePerFrame(VIDEO_FPS, 1, &this->rtDuration);

	HRESULT hr = MFCreateSinkWriterFromURL(this->fina.c_str(), NULL, NULL, &pSinkWriter);

	// Set the output media type.
	if (SUCCEEDED(hr))
	{
		hr = MFCreateMediaType(&pMediaTypeOut);  
	}
	if (SUCCEEDED(hr))
	{
		hr = pMediaTypeOut->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);	 
	}
	if (SUCCEEDED(hr))
	{
		hr = pMediaTypeOut->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_WMV3);
	}
	if (SUCCEEDED(hr))
	{
		hr = pMediaTypeOut->SetUINT32(MF_MT_AVG_BITRATE, this->bitRate);
	}

	if (SUCCEEDED(hr))
	{
		hr = pMediaTypeOut->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);   
	}
	if (SUCCEEDED(hr))
	{
		hr = MFSetAttributeSize(pMediaTypeOut, MF_MT_FRAME_SIZE, this->outputWidth, this->outputHeight);   
	}
	if (SUCCEEDED(hr))
	{
		hr = MFSetAttributeRatio(pMediaTypeOut, MF_MT_FRAME_RATE, VIDEO_FPS, 1);   
	}
	if (SUCCEEDED(hr))
	{
		hr = MFSetAttributeRatio(pMediaTypeOut, MF_MT_PIXEL_ASPECT_RATIO, 1, 1);   
	}

	if (SUCCEEDED(hr))
	{
		hr = pSinkWriter->AddStream(pMediaTypeOut, &streamIndex);   
	}

	// Set the input media type.
	if (SUCCEEDED(hr))
	{
		hr = MFCreateMediaType(&pMediaTypeIn);   
	}
	if (SUCCEEDED(hr))
	{
		hr = pMediaTypeIn->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);   
	}

	if (SUCCEEDED(hr))
	{
		hr = pMediaTypeIn->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB24);	 
	}
	if (SUCCEEDED(hr))
	{
		hr = pMediaTypeIn->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);   
	}
	if (SUCCEEDED(hr))
	{
		hr = MFSetAttributeSize(pMediaTypeIn, MF_MT_FRAME_SIZE, this->outputWidth, this->outputHeight);
	}
	if (SUCCEEDED(hr))
	{
		hr = MFSetAttributeRatio(pMediaTypeIn, MF_MT_FRAME_RATE, VIDEO_FPS, 1);   
	}
	if (SUCCEEDED(hr))
	{
		hr = MFSetAttributeRatio(pMediaTypeIn, MF_MT_PIXEL_ASPECT_RATIO, 1, 1);   
	}
	if (SUCCEEDED(hr))
	{
		hr = pSinkWriter->SetInputMediaType(streamIndex, pMediaTypeIn, NULL);   
	}
	
	// Tell the sink writer to start accepting data.
	if (SUCCEEDED(hr))
	{
		hr = pSinkWriter->BeginWriting();
	}

	SafeRelease(&pMediaTypeOut);
	SafeRelease(&pMediaTypeIn);
	return;
}

void MfVideoOutFile::CloseFile()
{
	if(this->pSinkWriter != NULL)
	{
		HRESULT hr = this->pSinkWriter->Finalize();
	}
	SafeRelease(&pSinkWriter);
}

void MfVideoOutFile::SendFrame(const char *imgIn, unsigned imgLen, const char *pxFmt, int width, int height)
{
	
	IMFSample *pSample = NULL;
	IMFMediaBuffer *pBuffer = NULL;

	const LONG cbWidth = BYTES_PER_TUPLE * this->outputWidth;
	const DWORD cbBuffer = cbWidth * this->outputHeight;

	if(this->pSinkWriter == NULL)
		this->OpenFile();

	BYTE *pData = NULL;

	// Create a new memory buffer.
	HRESULT hr = MFCreateMemoryBuffer(cbBuffer, &pBuffer);

	// Lock the buffer and copy the video frame to the buffer.
	if (SUCCEEDED(hr))
	{
		hr = pBuffer->Lock(&pData, NULL, NULL);
	}
	if (SUCCEEDED(hr))
	{
		if(strcmp(this->pxFmt.c_str(), pxFmt)!=0)
		{
			//std::cout << (long) pData << std::endl;

			unsigned int outBuffLen = cbBuffer;
			DecodeAndResizeFrame((const unsigned char *)imgIn, imgLen, pxFmt,
				width, height,
				this->pxFmt.c_str(),
				(unsigned char **)&pData,
				&outBuffLen, 
				this->outputWidth, this->outputHeight);

			//std::cout << (long) pData << std::endl;
		}
		else
		{
			DWORD cpyLen = imgLen;
			if(cbBuffer < cpyLen) cpyLen = cbBuffer;
			memcpy(pData, imgIn, cpyLen);
		}
	}
	if (pBuffer)
	{
		pBuffer->Unlock();
	}

	// Set the data length of the buffer.
	if (SUCCEEDED(hr))
	{
		hr = pBuffer->SetCurrentLength(cbBuffer);
	}

	// Create a media sample and add the buffer to the sample.
	if (SUCCEEDED(hr))
	{
		hr = MFCreateSample(&pSample);
	}
	if (SUCCEEDED(hr))
	{
		hr = pSample->AddBuffer(pBuffer);
	}

	// Set the time stamp and the duration.
	if (SUCCEEDED(hr))
	{
		hr = pSample->SetSampleTime(this->rtStart);
	}
	if (SUCCEEDED(hr))
	{
		hr = pSample->SetSampleDuration(this->rtDuration);
	}

	// Send the sample to the Sink Writer.
	if (SUCCEEDED(hr) && this->pSinkWriter != NULL)
	{
		hr = this->pSinkWriter->WriteSample(streamIndex, pSample);
	}

	this->rtStart += this->rtDuration;

	SafeRelease(&pSample);
	SafeRelease(&pBuffer);
}

void MfVideoOutFile::Stop()
{

}

int MfVideoOutFile::WaitForStop()
{
	return 1;
}

void MfVideoOutFile::SetOutputSize(int width, int height)
{
	if(this->pSinkWriter != NULL)
	{
		throw std::runtime_error("Set video size before opening video file");
	}
	this->outputWidth = width;
	this->outputHeight = height;
}

void MfVideoOutFile::SetOutputPxFmt(const char *fmt)
{
	if(this->pSinkWriter != NULL)
	{
		throw std::runtime_error("Set video format before opening video file");
	}
	if(strcmp(fmt,"BGR24")!=0)
		throw std::runtime_error("Only BGR24 is supported");
	this->pxFmt = fmt;
}

void MfVideoOutFile::Run()
{


}

//*******************************************************************************

void *MfVideoOut_File_Worker_thread(void *arg)
{
	class MfVideoOutFile *argobj = (class MfVideoOutFile*) arg;
	argobj->Run();

	return NULL;
}
