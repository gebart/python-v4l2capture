
#include <stdexcept>
#include <iostream>
#include <vector>
#include <string>
#include <map>
using namespace std;

#include <mfapi.h>
#include <Mferror.h>
#include <Shlwapi.h>

#include "mfvideoin.h"

#define MAX_DEVICE_ID_LEN 100

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

/*void SetSampleMetaData(IMFSourceReader *pReader, DWORD streamIndex, PyObject *out)
{
	//Set meta data in output object
	IMFMediaType *pCurrentType = NULL;
	LONG plStride = 0;
	GUID majorType=GUID_NULL, subType=GUID_NULL;
	UINT32 width = 0;
	UINT32 height = 0;

	HRESULT hr = pReader->GetCurrentMediaType(streamIndex, &pCurrentType);
	if(!SUCCEEDED(hr)) cout << "Error 3\n";
	BOOL isComp = FALSE;
	hr = pCurrentType->IsCompressedFormat(&isComp);
	PyDict_SetItemStringAndDeleteVar(out, "isCompressed", PyBool_FromLong(isComp));
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
	//if(subTypePtr!=0) wcout << "subtype\t" << subTypePtr << "\n";

	PyDict_SetItemStringAndDeleteVar(out, "isCompressed", PyBool_FromLong(isComp));
	if(typePtr!=NULL) PyDict_SetItemStringAndDeleteVar(out, "type", PyUnicode_FromWideChar(typePtr, wcslen(typePtr)));
	if(subTypePtr!=NULL) PyDict_SetItemStringAndDeleteVar(out, "subtype", PyUnicode_FromWideChar(subTypePtr, wcslen(subTypePtr)));
	if(!isComp) PyDict_SetItemStringAndDeleteVar(out, "stride", PyInt_FromLong(plStride));
	PyDict_SetItemStringAndDeleteVar(out, "width", PyInt_FromLong(width));
	PyDict_SetItemStringAndDeleteVar(out, "height", PyInt_FromLong(height));

}
*/

class SourceReaderCB : public IMFSourceReaderCallback
{
	//http://msdn.microsoft.com/en-us/library/windows/desktop/gg583871%28v=vs.85%29.aspx
public:
	LONG volatile m_nRefCount;
	CRITICAL_SECTION lock;
	int framePending;
	unsigned int maxNumFrames;

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
		maxNumFrames = 10;
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

    STDMETHODIMP OnReadSample(HRESULT hrStatus, DWORD dwStreamIndex,
        DWORD dwStreamFlags, LONGLONG llTimestamp, IMFSample *pSample)
	{
		//cout << "OnReadSample: " << llTimestamp << endl;
		EnterCriticalSection(&lock);

		if (pSample && this->frameBuff.size() < this->maxNumFrames)
		{
			char *buff = NULL;
			DWORD buffLen = SampleToStaticObj(pSample, &buff);
			//cout << (long) buff << "," << buffLen << endl;
			//if(buff!=NULL) delete [] buff;

			frameBuff.push_back(buff);
			frameLenBuff.push_back(buffLen);
			hrStatusBuff.push_back(hrStatus);
			dwStreamIndexBuff.push_back(dwStreamIndex);
			dwStreamFlagsBuff.push_back(dwStreamFlags);
			llTimestampBuff.push_back(llTimestamp);
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

	hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if(!SUCCEEDED(hr))
		throw std::runtime_error("CoInitializeEx failed");
}

WmfBase::~WmfBase()
{
	MFShutdown();

	CoUninitialize();
}


//***************************************************************************

MfVideoIn::MfVideoIn(const char *devNameIn) : WmfBase()
{
	this->asyncMode = 1;
	this->devName = devNameIn;
	this->reader = NULL;
	this->source = NULL;
	this->readerCallback = NULL;
}

MfVideoIn::~MfVideoIn()
{
	SafeRelease(&reader);
	SafeRelease(&source);
}

void MfVideoIn::Stop()
{
		
}

void MfVideoIn::WaitForStop()
{

}

void MfVideoIn::OpenDevice()
{

}

void MfVideoIn::SetFormat(const char *fmt, int width, int height)
{

}

void MfVideoIn::StartDevice(int buffer_count)
{

}

void MfVideoIn::StopDevice()
{

}

void MfVideoIn::CloseDevice()
{

}

int MfVideoIn::GetFrame(unsigned char **buffOut, class FrameMetaData *metaOut)
{
	return 0;
}

//***************************************************************

void *MfVideoIn_Worker_thread(void *arg)
{
	class MfVideoIn *argobj = (class MfVideoIn*) arg;
	argobj->Run();

	return NULL;
}

//******************************************************************

class WmfListDevices : public WmfBase
{
public:
	WmfListDevices() : WmfBase()
	{

	}

	virtual ~WmfListDevices()
	{

	}

	
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

	std::vector<std::vector<std::wstring> > ListDevices()
	{
		std::vector<std::vector<std::wstring> > out;

		IMFActivate **ppDevices = NULL;
		int count = this->EnumDevices(&ppDevices);
		cout << "count" << count << endl;
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

			cout << i << endl;
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
