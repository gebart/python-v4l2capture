
#include "mfvideooutfile.h"
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

const UINT32 VIDEO_WIDTH = 640;
const UINT32 VIDEO_HEIGHT = 480;
const UINT32 VIDEO_FPS = 25;
const UINT32 VIDEO_BIT_RATE = 800000;
const GUID   VIDEO_ENCODING_FORMAT = MFVideoFormat_WMV3;
const GUID   VIDEO_INPUT_FORMAT = MFVideoFormat_RGB24;
const UINT32 VIDEO_PELS = VIDEO_WIDTH * VIDEO_HEIGHT;
const UINT32 VIDEO_FRAME_COUNT = 20 * VIDEO_FPS;
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

	HRESULT hr = MFCreateSinkWriterFromURL(L"output.wmv", NULL, NULL, &pSinkWriter);

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
		hr = pMediaTypeOut->SetGUID(MF_MT_SUBTYPE, VIDEO_ENCODING_FORMAT);   
	}
	if (SUCCEEDED(hr))
	{
		hr = pMediaTypeOut->SetUINT32(MF_MT_AVG_BITRATE, VIDEO_BIT_RATE);   
	}

	if (SUCCEEDED(hr))
	{
		hr = pMediaTypeOut->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);   
	}
	if (SUCCEEDED(hr))
	{
		hr = MFSetAttributeSize(pMediaTypeOut, MF_MT_FRAME_SIZE, VIDEO_WIDTH, VIDEO_HEIGHT);   
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
		hr = pMediaTypeIn->SetGUID(MF_MT_SUBTYPE, VIDEO_INPUT_FORMAT);	 
	}
	if (SUCCEEDED(hr))
	{
		hr = pMediaTypeIn->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);   
	}
	if (SUCCEEDED(hr))
	{
		hr = MFSetAttributeSize(pMediaTypeIn, MF_MT_FRAME_SIZE, VIDEO_WIDTH, VIDEO_HEIGHT);   
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

	const LONG cbWidth = BYTES_PER_TUPLE * VIDEO_WIDTH;
	const DWORD cbBuffer = cbWidth * VIDEO_HEIGHT;

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
		for(int y = 0; y < height; y++)
		{
		for(int x = 0; x < width; x++)
		{
			BYTE *dstPx = &pData[x * BYTES_PER_TUPLE + y * cbWidth];
			const BYTE *srcPx = (const BYTE *)&imgIn[x * 3 + y * width * 3];
			dstPx[2] = srcPx[0]; //Red
			dstPx[1] = srcPx[1]; //Green
			dstPx[0] = srcPx[2]; //Blue
		}
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

}

void MfVideoOutFile::SetOutputPxFmt(const char *fmt)
{

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
