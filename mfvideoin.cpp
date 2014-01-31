
#include <stdexcept>
#include <iostream>
#include <vector>
#include <string>
#include <map>
using namespace std;

#include <mfapi.h>
#include <Mferror.h>
#include <Shlwapi.h>
#include <Dshow.h>

#include "mfvideoin.h"
#include "pixfmt.h"

//See also:
//https://github.com/Itseez/opencv/blob/master/modules/highgui/src/cap_msmf.cpp

#define MAX_DEVICE_ID_LEN 100
int EnumDevices(IMFActivate ***ppDevicesOut);

template <class T> void SafeRelease(T **ppT)
{
    if (*ppT)
    {
        (*ppT)->Release();
        *ppT = NULL;
    }
}

void PrintGuid(GUID guid)
{
	LPOLESTR lplpsz;
	StringFromCLSID(guid, &lplpsz);
	wcout << lplpsz << endl;
	CoTaskMemFree(lplpsz);
}

#ifndef IF_EQUAL_RETURN
#define IF_EQUAL_RETURN(param, val) if(val == param) return L#val
#endif

LPCWSTR GetGUIDNameConst(const GUID& guid)
{
	//http://msdn.microsoft.com/en-us/library/windows/desktop/ee663602%28v=vs.85%29.aspx
    IF_EQUAL_RETURN(guid, MF_MT_MAJOR_TYPE);
    IF_EQUAL_RETURN(guid, MF_MT_MAJOR_TYPE);
    IF_EQUAL_RETURN(guid, MF_MT_SUBTYPE);
    IF_EQUAL_RETURN(guid, MF_MT_ALL_SAMPLES_INDEPENDENT);
    IF_EQUAL_RETURN(guid, MF_MT_FIXED_SIZE_SAMPLES);
    IF_EQUAL_RETURN(guid, MF_MT_COMPRESSED);
    IF_EQUAL_RETURN(guid, MF_MT_SAMPLE_SIZE);
    IF_EQUAL_RETURN(guid, MF_MT_WRAPPED_TYPE);
    IF_EQUAL_RETURN(guid, MF_MT_AUDIO_NUM_CHANNELS);
    IF_EQUAL_RETURN(guid, MF_MT_AUDIO_SAMPLES_PER_SECOND);
    IF_EQUAL_RETURN(guid, MF_MT_AUDIO_FLOAT_SAMPLES_PER_SECOND);
    IF_EQUAL_RETURN(guid, MF_MT_AUDIO_AVG_BYTES_PER_SECOND);
    IF_EQUAL_RETURN(guid, MF_MT_AUDIO_BLOCK_ALIGNMENT);
    IF_EQUAL_RETURN(guid, MF_MT_AUDIO_BITS_PER_SAMPLE);
    IF_EQUAL_RETURN(guid, MF_MT_AUDIO_VALID_BITS_PER_SAMPLE);
    IF_EQUAL_RETURN(guid, MF_MT_AUDIO_SAMPLES_PER_BLOCK);
    IF_EQUAL_RETURN(guid, MF_MT_AUDIO_CHANNEL_MASK);
    IF_EQUAL_RETURN(guid, MF_MT_AUDIO_FOLDDOWN_MATRIX);
    IF_EQUAL_RETURN(guid, MF_MT_AUDIO_WMADRC_PEAKREF);
    IF_EQUAL_RETURN(guid, MF_MT_AUDIO_WMADRC_PEAKTARGET);
    IF_EQUAL_RETURN(guid, MF_MT_AUDIO_WMADRC_AVGREF);
    IF_EQUAL_RETURN(guid, MF_MT_AUDIO_WMADRC_AVGTARGET);
    IF_EQUAL_RETURN(guid, MF_MT_AUDIO_PREFER_WAVEFORMATEX);
    IF_EQUAL_RETURN(guid, MF_MT_AAC_PAYLOAD_TYPE);
    IF_EQUAL_RETURN(guid, MF_MT_AAC_AUDIO_PROFILE_LEVEL_INDICATION);
    IF_EQUAL_RETURN(guid, MF_MT_FRAME_SIZE);
    IF_EQUAL_RETURN(guid, MF_MT_FRAME_RATE);
    IF_EQUAL_RETURN(guid, MF_MT_FRAME_RATE_RANGE_MAX);
    IF_EQUAL_RETURN(guid, MF_MT_FRAME_RATE_RANGE_MIN);
    IF_EQUAL_RETURN(guid, MF_MT_PIXEL_ASPECT_RATIO);
    IF_EQUAL_RETURN(guid, MF_MT_DRM_FLAGS);
    IF_EQUAL_RETURN(guid, MF_MT_PAD_CONTROL_FLAGS);
    IF_EQUAL_RETURN(guid, MF_MT_SOURCE_CONTENT_HINT);
    IF_EQUAL_RETURN(guid, MF_MT_VIDEO_CHROMA_SITING);
    IF_EQUAL_RETURN(guid, MF_MT_INTERLACE_MODE);
    IF_EQUAL_RETURN(guid, MF_MT_TRANSFER_FUNCTION);
    IF_EQUAL_RETURN(guid, MF_MT_VIDEO_PRIMARIES);
    IF_EQUAL_RETURN(guid, MF_MT_CUSTOM_VIDEO_PRIMARIES);
    IF_EQUAL_RETURN(guid, MF_MT_YUV_MATRIX);
    IF_EQUAL_RETURN(guid, MF_MT_VIDEO_LIGHTING);
    IF_EQUAL_RETURN(guid, MF_MT_VIDEO_NOMINAL_RANGE);
    IF_EQUAL_RETURN(guid, MF_MT_GEOMETRIC_APERTURE);
    IF_EQUAL_RETURN(guid, MF_MT_MINIMUM_DISPLAY_APERTURE);
    IF_EQUAL_RETURN(guid, MF_MT_PAN_SCAN_APERTURE);
    IF_EQUAL_RETURN(guid, MF_MT_PAN_SCAN_ENABLED);
    IF_EQUAL_RETURN(guid, MF_MT_AVG_BITRATE);
    IF_EQUAL_RETURN(guid, MF_MT_AVG_BIT_ERROR_RATE);
    IF_EQUAL_RETURN(guid, MF_MT_MAX_KEYFRAME_SPACING);
    IF_EQUAL_RETURN(guid, MF_MT_DEFAULT_STRIDE);
    IF_EQUAL_RETURN(guid, MF_MT_PALETTE);
    IF_EQUAL_RETURN(guid, MF_MT_USER_DATA);
    IF_EQUAL_RETURN(guid, MF_MT_AM_FORMAT_TYPE);
    IF_EQUAL_RETURN(guid, MF_MT_MPEG_START_TIME_CODE);
    IF_EQUAL_RETURN(guid, MF_MT_MPEG2_PROFILE);
    IF_EQUAL_RETURN(guid, MF_MT_MPEG2_LEVEL);
    IF_EQUAL_RETURN(guid, MF_MT_MPEG2_FLAGS);
    IF_EQUAL_RETURN(guid, MF_MT_MPEG_SEQUENCE_HEADER);
    IF_EQUAL_RETURN(guid, MF_MT_DV_AAUX_SRC_PACK_0);
    IF_EQUAL_RETURN(guid, MF_MT_DV_AAUX_CTRL_PACK_0);
    IF_EQUAL_RETURN(guid, MF_MT_DV_AAUX_SRC_PACK_1);
    IF_EQUAL_RETURN(guid, MF_MT_DV_AAUX_CTRL_PACK_1);
    IF_EQUAL_RETURN(guid, MF_MT_DV_VAUX_SRC_PACK);
    IF_EQUAL_RETURN(guid, MF_MT_DV_VAUX_CTRL_PACK);
    IF_EQUAL_RETURN(guid, MF_MT_ARBITRARY_HEADER);
    IF_EQUAL_RETURN(guid, MF_MT_ARBITRARY_FORMAT);
    IF_EQUAL_RETURN(guid, MF_MT_IMAGE_LOSS_TOLERANT); 
    IF_EQUAL_RETURN(guid, MF_MT_MPEG4_SAMPLE_DESCRIPTION);
    IF_EQUAL_RETURN(guid, MF_MT_MPEG4_CURRENT_SAMPLE_ENTRY);
    IF_EQUAL_RETURN(guid, MF_MT_ORIGINAL_4CC); 
    IF_EQUAL_RETURN(guid, MF_MT_ORIGINAL_WAVE_FORMAT_TAG);

	//IF_EQUAL_RETURN(guid, FORMAT_VideoInfo); //Dshow dependent
	//IF_EQUAL_RETURN(guid, FORMAT_VideoInfo2);
    
    // Media types

    IF_EQUAL_RETURN(guid, MFMediaType_Audio);
    IF_EQUAL_RETURN(guid, MFMediaType_Video);
    IF_EQUAL_RETURN(guid, MFMediaType_Protected);
    IF_EQUAL_RETURN(guid, MFMediaType_SAMI);
    IF_EQUAL_RETURN(guid, MFMediaType_Script);
    IF_EQUAL_RETURN(guid, MFMediaType_Image);
    IF_EQUAL_RETURN(guid, MFMediaType_HTML);
    IF_EQUAL_RETURN(guid, MFMediaType_Binary);
    IF_EQUAL_RETURN(guid, MFMediaType_FileTransfer);

    IF_EQUAL_RETURN(guid, MFVideoFormat_AI44); //     FCC('AI44')
    IF_EQUAL_RETURN(guid, MFVideoFormat_ARGB32); //   D3DFMT_A8R8G8B8 
    IF_EQUAL_RETURN(guid, MFVideoFormat_AYUV); //     FCC('AYUV')
    IF_EQUAL_RETURN(guid, MFVideoFormat_DV25); //     FCC('dv25')
    IF_EQUAL_RETURN(guid, MFVideoFormat_DV50); //     FCC('dv50')
    IF_EQUAL_RETURN(guid, MFVideoFormat_DVH1); //     FCC('dvh1')
    IF_EQUAL_RETURN(guid, MFVideoFormat_DVSD); //     FCC('dvsd')
    IF_EQUAL_RETURN(guid, MFVideoFormat_DVSL); //     FCC('dvsl')
    IF_EQUAL_RETURN(guid, MFVideoFormat_H264); //     FCC('H264')
    IF_EQUAL_RETURN(guid, MFVideoFormat_I420); //     FCC('I420')
    IF_EQUAL_RETURN(guid, MFVideoFormat_IYUV); //     FCC('IYUV')
    IF_EQUAL_RETURN(guid, MFVideoFormat_M4S2); //     FCC('M4S2')
    IF_EQUAL_RETURN(guid, MFVideoFormat_MJPG);
    IF_EQUAL_RETURN(guid, MFVideoFormat_MP43); //     FCC('MP43')
    IF_EQUAL_RETURN(guid, MFVideoFormat_MP4S); //     FCC('MP4S')
    IF_EQUAL_RETURN(guid, MFVideoFormat_MP4V); //     FCC('MP4V')
    IF_EQUAL_RETURN(guid, MFVideoFormat_MPG1); //     FCC('MPG1')
    IF_EQUAL_RETURN(guid, MFVideoFormat_MSS1); //     FCC('MSS1')
    IF_EQUAL_RETURN(guid, MFVideoFormat_MSS2); //     FCC('MSS2')
    IF_EQUAL_RETURN(guid, MFVideoFormat_NV11); //     FCC('NV11')
    IF_EQUAL_RETURN(guid, MFVideoFormat_NV12); //     FCC('NV12')
    IF_EQUAL_RETURN(guid, MFVideoFormat_P010); //     FCC('P010')
    IF_EQUAL_RETURN(guid, MFVideoFormat_P016); //     FCC('P016')
    IF_EQUAL_RETURN(guid, MFVideoFormat_P210); //     FCC('P210')
    IF_EQUAL_RETURN(guid, MFVideoFormat_P216); //     FCC('P216')
    IF_EQUAL_RETURN(guid, MFVideoFormat_RGB24); //    D3DFMT_R8G8B8 
    IF_EQUAL_RETURN(guid, MFVideoFormat_RGB32); //    D3DFMT_X8R8G8B8 
    IF_EQUAL_RETURN(guid, MFVideoFormat_RGB555); //   D3DFMT_X1R5G5B5 
    IF_EQUAL_RETURN(guid, MFVideoFormat_RGB565); //   D3DFMT_R5G6B5 
    IF_EQUAL_RETURN(guid, MFVideoFormat_RGB8);
    IF_EQUAL_RETURN(guid, MFVideoFormat_UYVY); //     FCC('UYVY')
    IF_EQUAL_RETURN(guid, MFVideoFormat_v210); //     FCC('v210')
    IF_EQUAL_RETURN(guid, MFVideoFormat_v410); //     FCC('v410')
    IF_EQUAL_RETURN(guid, MFVideoFormat_WMV1); //     FCC('WMV1')
    IF_EQUAL_RETURN(guid, MFVideoFormat_WMV2); //     FCC('WMV2')
    IF_EQUAL_RETURN(guid, MFVideoFormat_WMV3); //     FCC('WMV3')
    IF_EQUAL_RETURN(guid, MFVideoFormat_WVC1); //     FCC('WVC1')
    IF_EQUAL_RETURN(guid, MFVideoFormat_Y210); //     FCC('Y210')
    IF_EQUAL_RETURN(guid, MFVideoFormat_Y216); //     FCC('Y216')
    IF_EQUAL_RETURN(guid, MFVideoFormat_Y410); //     FCC('Y410')
    IF_EQUAL_RETURN(guid, MFVideoFormat_Y416); //     FCC('Y416')
    IF_EQUAL_RETURN(guid, MFVideoFormat_Y41P);
    IF_EQUAL_RETURN(guid, MFVideoFormat_Y41T);
    IF_EQUAL_RETURN(guid, MFVideoFormat_YUY2); //     FCC('YUY2')
    IF_EQUAL_RETURN(guid, MFVideoFormat_YV12); //     FCC('YV12')
    IF_EQUAL_RETURN(guid, MFVideoFormat_YVYU);

    IF_EQUAL_RETURN(guid, MFAudioFormat_PCM); //              WAVE_FORMAT_PCM 
    IF_EQUAL_RETURN(guid, MFAudioFormat_Float); //            WAVE_FORMAT_IEEE_FLOAT 
    IF_EQUAL_RETURN(guid, MFAudioFormat_DTS); //              WAVE_FORMAT_DTS 
    IF_EQUAL_RETURN(guid, MFAudioFormat_Dolby_AC3_SPDIF); //  WAVE_FORMAT_DOLBY_AC3_SPDIF 
    IF_EQUAL_RETURN(guid, MFAudioFormat_DRM); //              WAVE_FORMAT_DRM 
    IF_EQUAL_RETURN(guid, MFAudioFormat_WMAudioV8); //        WAVE_FORMAT_WMAUDIO2 
    IF_EQUAL_RETURN(guid, MFAudioFormat_WMAudioV9); //        WAVE_FORMAT_WMAUDIO3 
    IF_EQUAL_RETURN(guid, MFAudioFormat_WMAudio_Lossless); // WAVE_FORMAT_WMAUDIO_LOSSLESS 
    IF_EQUAL_RETURN(guid, MFAudioFormat_WMASPDIF); //         WAVE_FORMAT_WMASPDIF 
    IF_EQUAL_RETURN(guid, MFAudioFormat_MSP1); //             WAVE_FORMAT_WMAVOICE9 
    IF_EQUAL_RETURN(guid, MFAudioFormat_MP3); //              WAVE_FORMAT_MPEGLAYER3 
    IF_EQUAL_RETURN(guid, MFAudioFormat_MPEG); //             WAVE_FORMAT_MPEG 
    IF_EQUAL_RETURN(guid, MFAudioFormat_AAC); //              WAVE_FORMAT_MPEG_HEAAC 
    IF_EQUAL_RETURN(guid, MFAudioFormat_ADTS); //             WAVE_FORMAT_MPEG_ADTS_AAC 

    return NULL;
}

HRESULT GetDefaultStride(IMFMediaType *pType, LONG *plStride)
{
	LONG lStride = 0;

	// Try to get the default stride from the media type.
	HRESULT hr = pType->GetUINT32(MF_MT_DEFAULT_STRIDE, (UINT32*)&lStride);

	if (FAILED(hr))
	{
		// Attribute not set. Try to calculate the default stride.

		GUID subtype = GUID_NULL;

		UINT32 width = 0;
		UINT32 height = 0;
		// Get the subtype and the image size.
		hr = pType->GetGUID(MF_MT_SUBTYPE, &subtype);
		if (FAILED(hr))
		{
			goto done;
		}
		hr = MFGetAttributeSize(pType, MF_MT_FRAME_SIZE, &width, &height);
		if (FAILED(hr))
		{
			goto done;
		}
		hr = MFGetStrideForBitmapInfoHeader(subtype.Data1, width, &lStride);
		if (FAILED(hr))
		{
			goto done;
		}

		// Set the attribute for later reference.
		(void)pType->SetUINT32(MF_MT_DEFAULT_STRIDE, UINT32(lStride));
	}

	if (SUCCEEDED(hr))
	{
		*plStride = lStride;
	}

done:
	return hr;
}

DWORD SampleToStaticObj(IMFSample *pSample, char **buff)
{
	if(*buff!=NULL)
		throw runtime_error("Buff ptr should be initially null");
	IMFMediaBuffer *ppBuffer = NULL;
	HRESULT hr = pSample->ConvertToContiguousBuffer(&ppBuffer);
	//cout << "ConvertToContiguousBuffer=" << SUCCEEDED(hr) << "\tstride="<< plStride << "\n";

	IMF2DBuffer *m_p2DBuffer = NULL;
	ppBuffer->QueryInterface(IID_IMF2DBuffer, (void**)&m_p2DBuffer);
	//cout << "IMF2DBuffer=" << (m_p2DBuffer != NULL) << "\n";

	DWORD pcbCurrentLength = 0;
	BYTE *ppbBuffer = NULL;
	DWORD pcbMaxLength = 0;

	if(SUCCEEDED(hr))
	{
		
		hr = ppBuffer->Lock(&ppbBuffer, &pcbMaxLength, &pcbCurrentLength);
		//cout << "pcbMaxLength="<< pcbMaxLength << "\tpcbCurrentLength=" <<pcbCurrentLength << "\n";

		//Return buffer as python format data
		*buff = new char[pcbCurrentLength];
		memcpy(*buff, ppbBuffer, pcbCurrentLength);

		ppBuffer->Unlock();
	}

	if(ppBuffer) ppBuffer->Release();
	return pcbCurrentLength;
}

class SourceReaderCB : public IMFSourceReaderCallback
{
	//http://msdn.microsoft.com/en-us/library/windows/desktop/gg583871%28v=vs.85%29.aspx
public:
	LONG volatile m_nRefCount;
	CRITICAL_SECTION lock;
	int framePending;
	unsigned int maxNumFrames;
	unsigned int droppedFrames;

	vector<char *> frameBuff;
	vector<DWORD> frameLenBuff;
	vector<HRESULT> hrStatusBuff;
	vector<DWORD> dwStreamIndexBuff;
    vector<DWORD> dwStreamFlagsBuff;
	vector<LONGLONG> llTimestampBuff;
	
	SourceReaderCB()
	{
		m_nRefCount = 0;
		framePending = 0;
		InitializeCriticalSection(&lock);
		maxNumFrames = 1;
		droppedFrames = 0;
	}

	virtual ~SourceReaderCB()
	{
		 DeleteCriticalSection(&lock);
		 for(unsigned int i=0; i<this->frameBuff.size(); i++)
			 delete [] this->frameBuff[i];
	}

	STDMETHODIMP QueryInterface(REFIID iid, void** ppv)
	{
		static const QITAB qit[] =
        {
            QITABENT(SourceReaderCB, IMFSourceReaderCallback),
            { 0 },
        };
        return QISearch(this, qit, iid, ppv);
	}

	void CheckForBufferOverflow()
	{
		//The lock should already be in use
		while(this->frameBuff.size() > this->maxNumFrames)
		{
			//Drop an old frame if buffer is starting to overflow
			char *frameToDrop = frameBuff[0];
			delete [] frameToDrop;
			frameToDrop = NULL;
			frameBuff.erase(frameBuff.begin());
			frameLenBuff.erase(frameLenBuff.begin());
			hrStatusBuff.erase(hrStatusBuff.begin());
			dwStreamIndexBuff.erase(dwStreamIndexBuff.begin());
			dwStreamFlagsBuff.erase(dwStreamFlagsBuff.begin());
			llTimestampBuff.erase(llTimestampBuff.begin());
			droppedFrames ++;
		}
	}

	void SetMaxBufferSize(unsigned maxBuffSizeIn)
	{
		EnterCriticalSection(&lock);
		this->maxNumFrames = maxBuffSizeIn;
		this->CheckForBufferOverflow();
		LeaveCriticalSection(&lock);
	}

    STDMETHODIMP OnReadSample(HRESULT hrStatus, DWORD dwStreamIndex,
        DWORD dwStreamFlags, LONGLONG llTimestamp, IMFSample *pSample)
	{
		//cout << "OnReadSample: " << llTimestamp << endl;
		EnterCriticalSection(&lock);

		if (pSample)
		{
			char *buff = NULL;
			DWORD buffLen = SampleToStaticObj(pSample, &buff);
			//cout << (long) buff << "," << buffLen << endl;
			//if(buff!=NULL) delete [] buff;

			//Always add frame to buffer
			frameBuff.push_back(buff);
			frameLenBuff.push_back(buffLen);
			hrStatusBuff.push_back(hrStatus);
			dwStreamIndexBuff.push_back(dwStreamIndex);
			dwStreamFlagsBuff.push_back(dwStreamFlags);
			llTimestampBuff.push_back(llTimestamp);

			this->CheckForBufferOverflow();
		}

		this->framePending = 0;
		LeaveCriticalSection(&lock);

		return S_OK;
	}

	STDMETHODIMP_(ULONG) AddRef()
	{
		return InterlockedIncrement(&m_nRefCount);
	}

    STDMETHODIMP_(ULONG) Release()
	{
		ULONG uCount = InterlockedDecrement(&m_nRefCount);
        if (uCount == 0)
        {
			//cout << "self destruct" << endl;
            delete this;
        }
        return uCount;
	}

	STDMETHODIMP OnEvent(DWORD, IMFMediaEvent *)
	{
		return S_OK;
	}

    STDMETHODIMP OnFlush(DWORD)
	{
		return S_OK;
	}

	void SetPending()
	{
		EnterCriticalSection(&lock);
		this->framePending = 1;
		LeaveCriticalSection(&lock);
	}

	int GetPending()
	{
		EnterCriticalSection(&lock);
		int pendingCopy = this->framePending;
		LeaveCriticalSection(&lock);
		return pendingCopy;
	}

	void WaitForFrame()
    {
		while(1)
		{
			EnterCriticalSection(&lock);
			int pendingCopy = this->framePending;
			LeaveCriticalSection(&lock);
			if (!pendingCopy) return;
			Sleep(10);
		}
    }

	int GetFrame(HRESULT *hrStatus, DWORD *dwStreamIndex,
        DWORD *dwStreamFlags, LONGLONG *llTimestamp, char **frame, DWORD *buffLen)
	{
		int ret = 0;
		*hrStatus = S_OK;
		*dwStreamIndex = 0;
		*dwStreamFlags = 0;
		*llTimestamp = 0;
		*frame = NULL;
		*buffLen = 0;

		EnterCriticalSection(&lock);
		if(this->frameBuff.size()>0)
		{
			*frame = frameBuff[0];
			*buffLen = frameLenBuff[0];
			*hrStatus = hrStatusBuff[0];
			*dwStreamIndex = dwStreamIndexBuff[0];
			*dwStreamFlags = dwStreamFlagsBuff[0];
			*llTimestamp = llTimestampBuff[0];

			this->frameBuff.erase(this->frameBuff.begin());
			this->frameLenBuff.erase(this->frameLenBuff.begin());
			this->hrStatusBuff.erase(this->hrStatusBuff.begin());
			this->dwStreamIndexBuff.erase(this->dwStreamIndexBuff.begin());
			this->dwStreamFlagsBuff.erase(this->dwStreamFlagsBuff.begin());
			this->llTimestampBuff.erase(this->llTimestampBuff.begin());
			ret = 1;
		}

		LeaveCriticalSection(&lock);
		return ret;
	}
   
};
//**************************************************************************

WmfBase::WmfBase() : Base_Video_In()
{
	HRESULT hr = MFStartup(MF_VERSION);
	if(!SUCCEEDED(hr))
		throw std::runtime_error("Media foundation startup failed");

	hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	if(hr == RPC_E_CHANGED_MODE)
		throw std::runtime_error("CoInitializeEx failed");
}

WmfBase::~WmfBase()
{
	MFShutdown();

	CoUninitialize();
}


//***************************************************************************

MfVideoIn::MfVideoIn(const wchar_t *devNameIn) : WmfBase()
{
	this->asyncMode = 1;
	this->devName = devNameIn;
	this->reader = NULL;
	this->source = NULL;
	this->readerCallback = NULL;
	this->stopping = 0;
	this->stopped = 0;

	this->openDevFlag = 0;
	this->startDevFlag = 0;
	this->stopDevFlag = 0;
	this->closeDevFlag = 0;
	this->maxBuffSize = 1;
	InitializeCriticalSection(&lock);
}

MfVideoIn::~MfVideoIn()
{
	this->WaitForStop();

	SafeRelease(&reader);
	SafeRelease(&source);
	DeleteCriticalSection(&lock);
}

void MfVideoIn::Stop()
{
	EnterCriticalSection(&lock);
	this->stopping = 1;
	LeaveCriticalSection(&lock);
}

void MfVideoIn::WaitForStop()
{
	this->Stop();

	int waiting = 1;
	while(waiting)
	{
		EnterCriticalSection(&lock);
		waiting = !this->stopped;
		LeaveCriticalSection(&lock);
		Sleep(10);
	}
}

void MfVideoIn::OpenDevice()
{
	cout << "MfVideoIn::OpenDevice()" << endl;
	EnterCriticalSection(&lock);
	this->openDevFlag = 1;
	LeaveCriticalSection(&lock);
}

void MfVideoIn::SetFormat(const char *fmt, int width, int height)
{

}

void MfVideoIn::StartDevice(int buffer_count)
{
	cout << "MfVideoIn::StartDevice()" << endl;
	EnterCriticalSection(&lock);
	this->startDevFlag = 1;
	LeaveCriticalSection(&lock);
}

void MfVideoIn::StopDevice()
{
	EnterCriticalSection(&lock);
	this->stopDevFlag = 1;
	LeaveCriticalSection(&lock);
}

void MfVideoIn::CloseDevice()
{
	cout << "MfVideoIn::CloseDevice()" << endl;
	EnterCriticalSection(&lock);
	this->closeDevFlag = 1;
	LeaveCriticalSection(&lock);
}

int MfVideoIn::GetFrame(unsigned char **buffOut, class FrameMetaData *metaOut)
{
	if(buffOut==NULL)
		throw runtime_error("Buffer ptr cannot be null");
	if(metaOut==NULL)
		throw runtime_error("Meta data pointer cannot be null");

	EnterCriticalSection(&lock);

	if(this->frameBuff.size() == 0)
	{
		LeaveCriticalSection(&lock);
		return 0;
	}

	unsigned char* currentBuff = (unsigned char *)this->frameBuff[0];
	std::string currentPixFmt = "Unknown";
	unsigned currentBuffLen = this->frameLenBuff[0];

	//wcout << this->majorTypeBuff[0] << "," << this->subTypeBuff[0] << endl;

	if(wcscmp(this->subTypeBuff[0].c_str(), L"MFVideoFormat_YUY2")==0)
		currentPixFmt = "YUYV"; //YUYV = YUY2
	if(wcscmp(this->subTypeBuff[0].c_str(), L"MFVideoFormat_RGB24")==0)
		currentPixFmt = "RGB24INV";

	//Do conversion to rgb
	unsigned char *buffConv = NULL;
	unsigned buffConvLen = 0;
	int widthTmp = this->widthBuff[0];
	int heightTmp = this->heightBuff[0];
	int ok = DecodeFrame(currentBuff, currentBuffLen, 
		currentPixFmt.c_str(),
		widthTmp, heightTmp,
		"RGB24",
		&buffConv,
		&buffConvLen);

	if(ok)
	{
		delete [] currentBuff; //Now unneeded
		currentBuff = buffConv;
		currentPixFmt = "RGB24";
		currentBuffLen = buffConvLen;
	}
	else
	{
		cout << "Cannot convert from pix format ";
		wcout << this->subTypeBuff[0] << endl;
	}
	
	*buffOut = currentBuff;
	metaOut->fmt = currentPixFmt;
	metaOut->width = this->widthBuff[0];
	metaOut->height = this->heightBuff[0];
	metaOut->buffLen = currentBuffLen;
	metaOut->sequence = 0;
	metaOut->tv_sec = (unsigned long)(this->llTimestampBuff[0] / 1e7); //in 100-nanosecond units
	metaOut->tv_usec = (unsigned long)((this->llTimestampBuff[0] - metaOut->tv_sec * 1e7) / 10);

	this->frameBuff.erase(this->frameBuff.begin());
	this->frameLenBuff.erase(this->frameLenBuff.begin());
	this->hrStatusBuff.erase(this->hrStatusBuff.begin());
	this->dwStreamIndexBuff.erase(this->dwStreamIndexBuff.begin());
	this->dwStreamFlagsBuff.erase(this->dwStreamFlagsBuff.begin());
	this->llTimestampBuff.erase(this->llTimestampBuff.begin());

	this->PopFrontMetaDataBuff();

	LeaveCriticalSection(&lock);

	return 1;
}

//***************************************************************

void MfVideoIn::Run()
{
	int running = 1;
	try
	{
	while(running)
	{
		EnterCriticalSection(&lock);
		running = !this->stopping || this->stopDevFlag || this->closeDevFlag;
		int openDevFlagTmp = this->openDevFlag;
		this->openDevFlag = 0;
		int startDevFlagTmp = this->startDevFlag;
		this->startDevFlag = 0;
		int stopDevFlagTmp = this->stopDevFlag;
		this->stopDevFlag = 0;
		int closeDevFlagTmp = this->closeDevFlag;
		this->closeDevFlag = 0;
		LeaveCriticalSection(&lock);
		if(!running) continue;

		if(openDevFlagTmp)
			this->OpenDeviceInternal();

		if(startDevFlagTmp)
			this->StartDeviceInternal();

		if(this->reader != NULL)
			this->ReadFramesInternal();

		if(stopDevFlagTmp)
			this->StopDeviceInternal();

		if(closeDevFlagTmp)
			this->CloseDeviceInternal();

		Sleep(10);
	}
	}
	catch(std::exception &err)
	{
		cout << err.what() << endl;
	}

	EnterCriticalSection(&lock);
	this->stopped = 1;
	LeaveCriticalSection(&lock);
}

void MfVideoIn::OpenDeviceInternal()
{
	//Check if source is already available
	if(this->source != NULL) 
		throw runtime_error("Device already open");
	
	//Open a new source
	IMFActivate **ppDevices = NULL;
	int count = EnumDevices(&ppDevices);
	int devIndex = -1;

	//Find device
	for(int i=0; i<count; i++)
	{
		IMFActivate *pActivate = ppDevices[i];
		wchar_t *symbolicLink = NULL;
		HRESULT hr = pActivate->GetAllocatedString(
			MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK,
			&symbolicLink,
			NULL
			);
		if(!SUCCEEDED(hr))
		{
			SafeRelease(ppDevices);
			throw std::runtime_error("GetAllocatedString failed");
		}

		if(wcscmp(symbolicLink, this->devName.c_str())==0)
		{
			devIndex = i;
		}
		CoTaskMemFree(symbolicLink);
	}

	if(devIndex == -1) 
		throw runtime_error("Device not found");

	IMFActivate *pActivate = ppDevices[devIndex];
		
	//Activate device object
	IMFMediaSource *sourceTmp = NULL;
	HRESULT hr = pActivate->ActivateObject(
		__uuidof(IMFMediaSource),
		(void**)&sourceTmp
		);
	if(!SUCCEEDED(hr))
	{
		SafeRelease(ppDevices);
		throw std::runtime_error("ActivateObject failed");
	}

	this->source = sourceTmp;

	SafeRelease(ppDevices);
}

void MfVideoIn::StartDeviceInternal()
{
	//Create reader
	IMFAttributes *pAttributes = NULL;
	HRESULT hr = MFCreateAttributes(&pAttributes, 1);
	if(!SUCCEEDED(hr))
		throw std::runtime_error("MFCreateAttributes failed");
		
	if(source==NULL)
		throw std::runtime_error("Source not open");

	//Set attributes for reader
	if(this->asyncMode)
	{
		this->readerCallback = new SourceReaderCB();

		hr = pAttributes->SetUnknown(MF_SOURCE_READER_ASYNC_CALLBACK, this->readerCallback);
	}

	IMFSourceReader *readerTmp = NULL;
	hr = MFCreateSourceReaderFromMediaSource(this->source, pAttributes, &readerTmp);
	if(!SUCCEEDED(hr))
	{
		SafeRelease(&pAttributes);
		throw std::runtime_error("MFCreateSourceReaderFromMediaSource failed");
	}

	this->reader = readerTmp;

	//this->GetMfControl(CameraControl_Exposure);
	//this->SetMfControl(CameraControl_Exposure, -3, 1);
	//this->GetMfControl(CameraControl_Exposure, 1);

	/*try
	{
		int ret = this->SetMfControl(CameraControl_Exposure, -3, CameraControl_Flags_Manual);
		std::cout << "ret" << ret << std::endl;
	}
	catch(std::runtime_error &err)
	{
		std::cout << "exception " << err.what() << std::endl;
	}*/
	
	SafeRelease(&pAttributes);
}

int MfVideoIn::GetMfControl(long prop, int range)
{
	long Min = 0;
	long Max = 0;
	long Step = 0;
	long Default = 0;
	long Flag = 0;

	IAMCameraControl *pProcControl = NULL;
	HRESULT hr = this->source->QueryInterface(IID_PPV_ARGS(&pProcControl));
	if(!SUCCEEDED(hr))
		throw runtime_error("IAMCameraControl interface not available");

	hr = pProcControl->GetRange(prop, &Min, &Max, &Step, &Default, &Flag);
	if(!SUCCEEDED(hr))
	{
		SafeRelease(&pProcControl);
		return 0;
	}

	if(range)
	{
		std::cout << "Min " << Min << std::endl;
		std::cout << "Max " << Max << std::endl;
		std::cout << "Step " << Step << std::endl;
		std::cout << "Default " << Default << std::endl;
		std::cout << "Allowed Flag " << Flag << std::endl;
	}

	long val = 0, flags = 0;
	hr = pProcControl->Get(prop, &val, &flags);

	std::cout << "Current Value " << prop << " " << val << std::endl;
	std::cout << "Current Flag " << prop << " " << flags << std::endl;

	SafeRelease(&pProcControl);
	return SUCCEEDED(hr);
}

int MfVideoIn::SetMfControl(long prop, long value, long flags)
{
	if(flags==0)
		flags = CameraControl_Flags_Manual;

	IAMCameraControl *pProcControl = NULL;
	HRESULT hr = this->source->QueryInterface(IID_PPV_ARGS(&pProcControl));
	if(!SUCCEEDED(hr))
		throw runtime_error("IAMCameraControl interface not available");

	hr = pProcControl->Set(prop, value, flags);

	SafeRelease(&pProcControl);
	return SUCCEEDED(hr);

}

int MfVideoIn::GetMfParameter(long param, int range)
{
	long Min = 0;
	long Max = 0;
	long Step = 0;
	long Default = 0;
	long Flag = 0;

	IAMVideoProcAmp *pProcAmp = NULL;
	HRESULT hr = this->source->QueryInterface(IID_PPV_ARGS(&pProcAmp));
	if(!SUCCEEDED(hr))
		throw runtime_error("IAMCameraControl interface not available");

	hr = pProcAmp->GetRange(param, &Min, &Max, &Step, &Default, &Flag);
	if(!SUCCEEDED(hr))
	{
		SafeRelease(&pProcAmp);
		return 0;
	}

	if(range)
	{
		std::cout << "param "<< param << " Min " << Min << std::endl;
		std::cout << "param "<< param << " Max " << Max << std::endl;
		std::cout << "param "<< param << " Step " << Step << std::endl;
		std::cout << "param "<< param << " Default " << Default << std::endl;
		std::cout << "param "<< param << " Allowed Flag " << Flag << std::endl;
	}

	long val = 0, flags = 0;
	hr = pProcAmp->Get(param, &val, &flags);

	std::cout << "param "<< param << " Current Value " << val << std::endl;
	std::cout << "param "<< param << " Current Flag " << flags << std::endl;

	SafeRelease(&pProcAmp);
	return SUCCEEDED(hr);
}

int MfVideoIn::SetMfParameter(long param, long value, long flags)
{
	if(flags==0)
		flags = CameraControl_Flags_Manual;

	IAMVideoProcAmp *pProcAmp = NULL;
	HRESULT hr = this->source->QueryInterface(IID_PPV_ARGS(&pProcAmp));
	if(!SUCCEEDED(hr))
		throw runtime_error("IAMCameraControl interface not available");

	hr = pProcAmp->Set(param, value, flags);

	SafeRelease(&pProcAmp);
	return SUCCEEDED(hr);

}

void MfVideoIn::SetSampleMetaData(DWORD streamIndex)
{
	//Set meta data in output object
	IMFMediaType *pCurrentType = NULL;
	LONG plStride = 0;
	GUID majorType=GUID_NULL, subType=GUID_NULL;
	UINT32 width = 0;
	UINT32 height = 0;

	HRESULT hr = this->reader->GetCurrentMediaType(streamIndex, &pCurrentType);
	if(!SUCCEEDED(hr)) cout << "Error 3\n";
	BOOL isComp = FALSE;
	hr = pCurrentType->IsCompressedFormat(&isComp);
	hr = pCurrentType->GetGUID(MF_MT_MAJOR_TYPE, &majorType);
	LPCWSTR typePtr = GetGUIDNameConst(majorType);
	if(!SUCCEEDED(hr)) cout << "Error 4\n";
	hr = pCurrentType->GetGUID(MF_MT_SUBTYPE, &subType);
	if(!SUCCEEDED(hr)) cout << "Error 5\n";
	int isVideo = (majorType==MFMediaType_Video);
	if(isVideo)
	{
		GetDefaultStride(pCurrentType, &plStride);
		hr = MFGetAttributeSize(pCurrentType, MF_MT_FRAME_SIZE, &width, &height);
		if(!SUCCEEDED(hr)) cout << "Error 20\n";
	}

	LPCWSTR subTypePtr = GetGUIDNameConst(subType);
	
	this->plStrideBuff.push_back(plStride);
	this->majorTypeBuff.push_back(typePtr);
	this->subTypeBuff.push_back(subTypePtr);
	this->widthBuff.push_back(width);
	this->heightBuff.push_back(height);
	this->isCompressedBuff.push_back(isComp);

	SafeRelease(&pCurrentType);
}

void MfVideoIn::PopFrontMetaDataBuff()

{
	if(this->plStrideBuff.size()>0) this->plStrideBuff.erase(this->plStrideBuff.begin());
	if(this->majorTypeBuff.size()>0) this->majorTypeBuff.erase(this->majorTypeBuff.begin());
	this->subTypeBuff.erase(this->subTypeBuff.begin());
	this->widthBuff.erase(this->widthBuff.begin());
	this->heightBuff.erase(this->heightBuff.begin());
	this->isCompressedBuff.erase(this->isCompressedBuff.begin());
}

void MfVideoIn::ReadFramesInternal()
{
	//Check if reader is ready
	if(this->reader == NULL)
		throw std::runtime_error("Reader not ready for this source");

	HRESULT hr = S_OK;
	IMFSample *pSample = NULL;
	DWORD streamIndex=0, flags=0;
	LONGLONG llTimeStamp=0;

	if(this->asyncMode)
	{
		if(!this->readerCallback->GetPending())
		{
			hr = this->reader->ReadSample(
				MF_SOURCE_READER_ANY_STREAM,    // Stream index.
				0, NULL, NULL, NULL, NULL
				);
			this->readerCallback->SetPending();
		}

		HRESULT hrStatus = S_OK;
		DWORD dwStreamIndex = 0;
		DWORD dwStreamFlags = 0; 
		LONGLONG llTimestamp = 0;
		char *frame = NULL;
		DWORD buffLen = 0;

		int found = this->readerCallback->GetFrame(&hrStatus, &dwStreamIndex,
			&dwStreamFlags, &llTimestamp, &frame, &buffLen);

		//cout << (long) frame << "," << buffLen << endl;
		if(found)
		{
			if((frame == NULL) != (buffLen == 0))
				throw runtime_error("Frame buffer corruption detected");

			EnterCriticalSection(&lock);

			//Ensure the buffer does not overflow
			while(this->frameBuff.size() >= this->maxBuffSize)
			{
				char *frameToDrop = this->frameBuff[0];
				delete [] frameToDrop;
				frameToDrop = NULL;
				this->frameBuff.erase(this->frameBuff.begin());
				this->frameLenBuff.erase(this->frameLenBuff.begin());
				this->hrStatusBuff.erase(this->hrStatusBuff.begin());
				this->dwStreamIndexBuff.erase(this->dwStreamIndexBuff.begin());
				this->dwStreamFlagsBuff.erase(this->dwStreamFlagsBuff.begin());
				this->llTimestampBuff.erase(this->llTimestampBuff.begin());

				this->PopFrontMetaDataBuff();
			}

			//Copy frame to output buffer
			if(this->frameBuff.size() < this->maxBuffSize)
			{
				this->frameBuff.push_back(frame);
				this->frameLenBuff.push_back(buffLen);
				this->hrStatusBuff.push_back(hrStatus);
				this->dwStreamIndexBuff.push_back(dwStreamIndex);
				this->dwStreamFlagsBuff.push_back(dwStreamFlags);
				this->llTimestampBuff.push_back(llTimestamp);

				this->SetSampleMetaData(dwStreamIndex);
			}
			else
			{
				delete [] frame;
			}

			LeaveCriticalSection(&lock);

			//for(long i=VideoProcAmp_Brightness;i<=VideoProcAmp_Gain;i++)
			//	this->GetMfParameter(i, 0);
			/*int ret = this->SetMfControl(CameraControl_Exposure, -3, CameraControl_Flags_Manual);
			std::cout << "ret" << ret << std::endl;
			this->GetMfControl(CameraControl_Exposure, 1);
			this->SetMfParameter(VideoProcAmp_Gain, 0, VideoProcAmp_Flags_Auto);
			std::cout << "ret" << ret << std::endl;
			this->GetMfParameter(VideoProcAmp_Gain, 0);
			this->SetMfParameter(VideoProcAmp_Gamma, 72, VideoProcAmp_Flags_Auto);
			std::cout << "ret" << ret << std::endl;
			this->GetMfParameter(VideoProcAmp_Gamma, 0);*/
			//for(long i=CameraControl_Pan;i<=CameraControl_Focus;i++)
			//	this->GetMfControl(i, 0);

			return;
		}
		else
			return;
	}
	else
	{
		hr = this->reader->ReadSample(
			MF_SOURCE_READER_ANY_STREAM,    // Stream index.
			0,                              // Flags.
			&streamIndex,                   // Receives the actual stream index. 
			&flags,                         // Receives status flags.
			&llTimeStamp,                   // Receives the time stamp.
			&pSample                        // Receives the sample or NULL.
			);
		
		if (FAILED(hr))
		{
			return;
		}

		if(pSample!=NULL)
		{
			char *frame = NULL;
			DWORD buffLen = SampleToStaticObj(pSample, &frame);

			EnterCriticalSection(&lock);

			//Ensure the buffer does not overflow
			while(this->frameBuff.size() >= this->maxBuffSize)
			{
				this->frameBuff.erase(this->frameBuff.begin());
				this->frameLenBuff.erase(this->frameLenBuff.begin());
				this->hrStatusBuff.erase(this->hrStatusBuff.begin());
				this->dwStreamIndexBuff.erase(this->dwStreamIndexBuff.begin());
				this->dwStreamFlagsBuff.erase(this->dwStreamFlagsBuff.begin());
				this->llTimestampBuff.erase(this->llTimestampBuff.begin());

				this->PopFrontMetaDataBuff();
			}

			//Copy frame to output buffer
			if(this->frameBuff.size() < this->maxBuffSize)
			{
				this->frameBuff.push_back(frame);
				this->frameLenBuff.push_back(buffLen);
				this->hrStatusBuff.push_back(hr);
				this->dwStreamIndexBuff.push_back(streamIndex);
				this->dwStreamFlagsBuff.push_back(flags);
				this->llTimestampBuff.push_back(llTimeStamp);

				this->SetSampleMetaData(streamIndex);
			}
			else
			{
				delete [] frame;
			}

			LeaveCriticalSection(&lock);	

			pSample->Release();
			return;
		}

		if(pSample) pSample->Release();
	}

}

void MfVideoIn::StopDeviceInternal()
{
	if(this->reader == NULL)
		throw runtime_error("Device is not running");

	//Shut down reader
	SafeRelease(&this->reader);

	//Reader callback seems to automatically delete
	this->readerCallback = NULL;

}

void MfVideoIn::CloseDeviceInternal()
{
	if(this->source == NULL)
		throw runtime_error("Device is not open");

	//Shut down source
	SafeRelease(&this->source);
}

//***************************************************************

void *MfVideoIn_Worker_thread(void *arg)
{
	class MfVideoIn *argobj = (class MfVideoIn*) arg;
	argobj->Run();

	return NULL;
}

//******************************************************************

int EnumDevices(IMFActivate ***ppDevicesOut)
{
	//Warning: the result from this function must be manually freed!

	//Allocate memory to store devices
	IMFAttributes *pAttributes = NULL;
	*ppDevicesOut = NULL;
	HRESULT hr = MFCreateAttributes(&pAttributes, 1);
	if(!SUCCEEDED(hr))
		throw std::runtime_error("MFCreateAttributes failed");

	hr = pAttributes->SetGUID(
           MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
           MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID
           );
	if(!SUCCEEDED(hr))
	{
		SafeRelease(&pAttributes);
		throw std::runtime_error("SetGUID failed");
	}

	//Get list of devices from media foundation
	UINT32 count;
	hr = MFEnumDeviceSources(pAttributes, ppDevicesOut, &count);
	if(!SUCCEEDED(hr))
	{
		SafeRelease(&pAttributes);
		throw std::runtime_error("MFEnumDeviceSources failed");
	}

	SafeRelease(&pAttributes);
	return count;
}

class WmfListDevices : public WmfBase
{
public:
	WmfListDevices() : WmfBase()
	{

	}

	virtual ~WmfListDevices()
	{

	}

	std::vector<std::vector<std::wstring> > ListDevices()
	{
		std::vector<std::vector<std::wstring> > out;

		IMFActivate **ppDevices = NULL;
		int count = EnumDevices(&ppDevices);
		
		//For each device
		for(int i=0; i<count; i++)
		{
			IMFActivate *pActivate = ppDevices[i];
			wchar_t *vd_pFriendlyName = NULL;

			//Get friendly names for devices
			HRESULT hr = pActivate->GetAllocatedString(
				MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME,
				&vd_pFriendlyName,
				NULL
				);
			if(!SUCCEEDED(hr))
			{
				SafeRelease(ppDevices);
				CoTaskMemFree(vd_pFriendlyName);
				throw std::runtime_error("GetAllocatedString failed");
			}

			wchar_t *symbolicLink = NULL;
			hr = pActivate->GetAllocatedString(
				MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK,
				&symbolicLink,
				NULL
				);
			if(!SUCCEEDED(hr))
			{
				SafeRelease(ppDevices);
				CoTaskMemFree(vd_pFriendlyName);
				CoTaskMemFree(symbolicLink);
				throw std::runtime_error("GetAllocatedString failed");
			}

			std::vector<std::wstring> src;
			src.push_back(symbolicLink);
			src.push_back(vd_pFriendlyName);
			out.push_back(src);

			CoTaskMemFree(vd_pFriendlyName);
			CoTaskMemFree(symbolicLink);
		}

		if(ppDevices) 
			SafeRelease(ppDevices);

		return out;
	}
};

std::vector<std::vector<std::wstring> > List_in_devices()
{
	class WmfListDevices wmfListDevices;
	std::vector<std::vector<std::wstring> > out = wmfListDevices.ListDevices();

	return out;
}
