
#include "mfvideooutfile.h"
#include "pixfmt.h"
#include <iostream>
#include <stdexcept>
#include <mfapi.h>
#include <Mferror.h>
#include <comdef.h>
using namespace std;

template <class T> void SafeRelease(T **ppT)
{
	if (*ppT)
	{
		(*ppT)->Release();
		*ppT = NULL;
	}
}

std::string HrCodeToStdString(HRESULT hr)
{
	std::string out;
	_com_error err(hr);
	LPCTSTR hrErrMsg = err.ErrorMessage();
	
#ifdef  UNICODE 
	size_t errsize = wcstombs(NULL, hrErrMsg, 0);
	char* tmpStr = new char[errsize + 1];
	wcstombs(tmpStr, hrErrMsg, errsize + 1 );
	out = hrErrMsg;
	delete tmpStr;
#else
	out = hrErrMsg;
#endif
	return out;
}

FILETIME GetTimeNow()
{
	SYSTEMTIME systime;
	GetSystemTime(&systime);
	FILETIME time;
	SystemTimeToFileTime(&systime, &time);
	return time;
}

double SubtractTimes(FILETIME first, FILETIME second)
{
	LONGLONG diffInTicks =
		reinterpret_cast<LARGE_INTEGER*>(&first)->QuadPart -
		reinterpret_cast<LARGE_INTEGER*>(&second)->QuadPart;
	double diffInSec = diffInTicks / (double)1e7;
	return diffInSec;
}

void SetTimeToZero(FILETIME &t)
{
	t.dwLowDateTime = 0;
	t.dwHighDateTime = 0;
}

bool TimeIsZero(FILETIME &t)
{
	if (t.dwLowDateTime != 0) return 0;
	return t.dwHighDateTime == 0;
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
	this->forceFrameRateFps = 0;
	this->prevFrameDuration = 0;
	SetTimeToZero(this->startVideoTime);
}

MfVideoOutFile::~MfVideoOutFile()
{
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
	this->rtDuration = 1;
	std::string errMsg;
	if(this->forceFrameRateFps > 0)
		MFFrameRateToAverageTimePerFrame(this->forceFrameRateFps, 1, &this->rtDuration);

	HRESULT hr = MFCreateSinkWriterFromURL(this->fina.c_str(), NULL, NULL, &pSinkWriter);
	if (!SUCCEEDED(hr))
	{
		errMsg = "MFCreateSinkWriterFromURL failed";
	}

	// Set the output media type.
	if (SUCCEEDED(hr))
	{
		hr = MFCreateMediaType(&pMediaTypeOut);
		if (!SUCCEEDED(hr)) errMsg = "MFCreateMediaType failed";
	}
	if (SUCCEEDED(hr))
	{
		hr = pMediaTypeOut->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);	 
		if (!SUCCEEDED(hr)) errMsg = "SetGUID MF_MT_MAJOR_TYPE failed";
	}
	if (SUCCEEDED(hr))
	{
		//hr = pMediaTypeOut->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_WMV3);
		hr = pMediaTypeOut->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_H264);
		if (!SUCCEEDED(hr)) errMsg = "SetGUID MF_MT_SUBTYPE failed";
	}
	if (SUCCEEDED(hr))
	{
		hr = pMediaTypeOut->SetUINT32(MF_MT_AVG_BITRATE, this->bitRate);
		if (!SUCCEEDED(hr)) errMsg = "Set MF_MT_AVG_BITRATE failed";
	}

	if (SUCCEEDED(hr))
	{
		hr = pMediaTypeOut->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);   
		if (!SUCCEEDED(hr)) errMsg = "Set MF_MT_INTERLACE_MODE failed";
	}
	if (SUCCEEDED(hr))
	{
		hr = MFSetAttributeSize(pMediaTypeOut, MF_MT_FRAME_SIZE, this->outputWidth, this->outputHeight);   
		if (!SUCCEEDED(hr)) errMsg = "Set MF_MT_FRAME_SIZE failed";
	}
	if (SUCCEEDED(hr) && this->forceFrameRateFps > 0)
	{
		hr = MFSetAttributeRatio(pMediaTypeOut, MF_MT_FRAME_RATE, this->forceFrameRateFps, 1);   
		if (!SUCCEEDED(hr)) errMsg = "Set MF_MT_FRAME_RATE failed";
	}
	if (SUCCEEDED(hr))
	{
		hr = MFSetAttributeRatio(pMediaTypeOut, MF_MT_PIXEL_ASPECT_RATIO, 1, 1);   
		if (!SUCCEEDED(hr)) errMsg = "Set MF_MT_PIXEL_ASPECT_RATIO failed";
	}

	if (SUCCEEDED(hr))
	{
		hr = pSinkWriter->AddStream(pMediaTypeOut, &streamIndex);   
		if (!SUCCEEDED(hr)) errMsg = "AddStream failed";
	}

	// Get supported types of output
	IMFTransform *transform = NULL;
	if (SUCCEEDED(hr))
	{
		hr = pSinkWriter->GetServiceForStream(streamIndex, GUID_NULL, IID_IMFTransform, (LPVOID*)&transform);
		if (!SUCCEEDED(hr))
		{
			errMsg = "GetServiceForStream failed: ";
			std::string hrErrStr = HrCodeToStdString(hr);
			errMsg += hrErrStr;
		}
	}

	if (SUCCEEDED(hr) && transform != NULL)
	{
		IMFMediaType *fmtType;
		hr = transform->GetInputAvailableType(streamIndex, 0, &fmtType);
		std::cout << SUCCEEDED(hr) << "," << (LONG)fmtType << std::endl;
		if (!SUCCEEDED(hr)) errMsg = "GetInputAvailableType failed";
	}

	// Set the input media type.
	if (SUCCEEDED(hr))
	{
		hr = MFCreateMediaType(&pMediaTypeIn);   
		if (!SUCCEEDED(hr)) errMsg = "Set MFCreateMediaType failed";
	}
	if (SUCCEEDED(hr))
	{
		hr = pMediaTypeIn->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);   
		if (!SUCCEEDED(hr)) errMsg = "Set MF_MT_MAJOR_TYPE failed";
	}

	if (SUCCEEDED(hr))
	{
		hr = pMediaTypeIn->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB24);	 
		if (!SUCCEEDED(hr)) errMsg = "Set MF_MT_SUBTYPE failed";
	}
	if (SUCCEEDED(hr))
	{
		hr = pMediaTypeIn->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);   
		if (!SUCCEEDED(hr)) errMsg = "Set MF_MT_INTERLACE_MODE failed";
	}
	if (SUCCEEDED(hr))
	{
		hr = MFSetAttributeSize(pMediaTypeIn, MF_MT_FRAME_SIZE, this->outputWidth, this->outputHeight);
		if (!SUCCEEDED(hr)) errMsg = "Set MF_MT_FRAME_SIZE failed";
	}
	if (SUCCEEDED(hr) && this->forceFrameRateFps > 0)
	{
		hr = MFSetAttributeRatio(pMediaTypeIn, MF_MT_FRAME_RATE, this->forceFrameRateFps, 1);   
		if (!SUCCEEDED(hr)) errMsg = "Set MF_MT_FRAME_RATE failed";
	}
	if (SUCCEEDED(hr))
	{
		hr = MFSetAttributeRatio(pMediaTypeIn, MF_MT_PIXEL_ASPECT_RATIO, 1, 1);   
		if (!SUCCEEDED(hr)) errMsg = "Set MF_MT_PIXEL_ASPECT_RATIO failed";
	}
	if (SUCCEEDED(hr))
	{
		hr = pSinkWriter->SetInputMediaType(streamIndex, pMediaTypeIn, NULL);   
		if (!SUCCEEDED(hr)) errMsg = "SetInputMediaType failed";
		if(hr == MF_E_INVALIDMEDIATYPE) errMsg.append(": MF_E_INVALIDMEDIATYPE");
		if(hr == MF_E_INVALIDSTREAMNUMBER) errMsg.append(": MF_E_INVALIDSTREAMNUMBER");
		if(hr == MF_E_TOPO_CODEC_NOT_FOUND) errMsg.append(": MF_E_TOPO_CODEC_NOT_FOUND");
	}
	
	// Tell the sink writer to start accepting data.
	if (SUCCEEDED(hr))
	{
		hr = pSinkWriter->BeginWriting();
		if (!SUCCEEDED(hr)) errMsg = "BeginWriting failed";
	}

	SafeRelease(&pMediaTypeOut);
	SafeRelease(&pMediaTypeIn);

	if(errMsg.size() > 0)
	{
		throw runtime_error(errMsg);
	}
	return;
}

void MfVideoOutFile::CloseFile()
{
	this->CopyFromBufferToOutFile(1);

	if(this->pSinkWriter != NULL)
	{
		HRESULT hr = this->pSinkWriter->Finalize();
	}
	SafeRelease(&pSinkWriter);
}

void MfVideoOutFile::SendFrame(const char *imgIn, unsigned imgLen, const char *pxFmt, int width, int height)
{
	if(this->pSinkWriter == NULL)
		this->OpenFile();

	FILETIME timeNow = GetTimeNow();
	if(TimeIsZero(this->startVideoTime))
	{
		this->startVideoTime = timeNow;
	}

	//Time since video start
	unsigned long elapseSec = 0;
	unsigned long elapseUSec = 0;
	if(this->forceFrameRateFps > 0)
	{
		//Fixed frame rate
		elapseSec = (unsigned long)(this->rtStart / 1e7);
		elapseUSec = (unsigned long)((this->rtStart - elapseSec * 1e7)/10. + 0.5);
		this->rtStart += this->rtDuration;
	}
	else
	{
		//Real time frames
		double elapse = SubtractTimes(timeNow, this->startVideoTime);
		elapseSec = (unsigned long)elapse;
		elapseUSec = (unsigned long)(((elapse - (double)elapseSec) / (double)1e6) + 0.5);
	}

	//Add frame to output buffer
	class FrameMetaData tmp;
	this->outBufferMeta.push_back(tmp);
	class FrameMetaData &meta = this->outBufferMeta[this->outBufferMeta.size()-1];
	meta.fmt = pxFmt;
	meta.width = width;
	meta.height = height;
	meta.buffLen = imgLen;
	meta.tv_sec = elapseSec;
	meta.tv_usec = elapseUSec;
	std::string img(imgIn, imgLen);
	this->outBuffer.push_back(img);

	this->CopyFromBufferToOutFile(0);
}

void MfVideoOutFile::CopyFromBufferToOutFile(int lastFrame)
{
	if(this->outBuffer.size() < 2 && !lastFrame)
		return;
	if(this->outBuffer.size() == 0)
		return;

	std::string &frame = this->outBuffer[0];
	class FrameMetaData &meta = this->outBufferMeta[0];
	class FrameMetaData *metaNext = NULL;
	if(this->outBuffer.size() >= 2)
		metaNext = &this->outBufferMeta[1];

	IMFSample *pSample = NULL;
	IMFMediaBuffer *pBuffer = NULL;

	const LONG cbWidth = BYTES_PER_TUPLE * this->outputWidth;
	const DWORD cbBuffer = cbWidth * this->outputHeight;

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
		if(strcmp(this->pxFmt.c_str(), meta.fmt.c_str())!=0)
		{
			//std::cout << (long) pData << std::endl;

			unsigned int outBuffLen = cbBuffer;
			DecodeAndResizeFrame((const unsigned char *)frame.c_str(), frame.size(), meta.fmt.c_str(),
				meta.width, meta.height,
				this->pxFmt.c_str(),
				(unsigned char **)&pData,
				&outBuffLen, 
				this->outputWidth, this->outputHeight);

			//std::cout << (long) pData << std::endl;
		}
		else
		{
			DWORD cpyLen = frame.size();
			if(cbBuffer < cpyLen) cpyLen = cbBuffer;
			memcpy(pData, frame.c_str(), cpyLen);
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
	LONGLONG frameTime = (LONGLONG)meta.tv_sec * (LONGLONG)1e7 + (LONGLONG)meta.tv_usec * 10;
	LONGLONG duration = 0;
	if(metaNext!=NULL)
	{
		LONGLONG frameTimeNext = (LONGLONG)metaNext->tv_sec * (LONGLONG)1e7 + (LONGLONG)metaNext->tv_usec * 10;
		duration = frameTimeNext - frameTime;
	}
	else
	{
		duration = this->prevFrameDuration;
		if(duration == 0) duration = (LONGLONG)1e7; //Avoid zero duration frames
	}

	if (SUCCEEDED(hr))
	{
		hr = pSample->SetSampleTime(frameTime);
	}
	if (SUCCEEDED(hr))
	{
		hr = pSample->SetSampleDuration(duration);
	}

	// Send the sample to the Sink Writer.
	if (SUCCEEDED(hr) && this->pSinkWriter != NULL)
	{
		hr = this->pSinkWriter->WriteSample(streamIndex, pSample);
	}

	SafeRelease(&pSample);
	SafeRelease(&pBuffer);

	this->outBuffer.erase(this->outBuffer.begin());
	this->outBufferMeta.erase(this->outBufferMeta.begin());
	this->prevFrameDuration = duration;
}

void MfVideoOutFile::Stop()
{
	this->CloseFile();

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

void MfVideoOutFile::SetFrameRate(UINT32 frameRateIn)
{
	if(this->pSinkWriter != NULL)
	{
		throw std::runtime_error("Set video parameters before opening video file");
	}
	this->forceFrameRateFps = frameRateIn;
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
