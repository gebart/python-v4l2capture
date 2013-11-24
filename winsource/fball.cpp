
#include <streams.h>
#include <olectl.h>
#include <initguid.h>
#include <new>
#include "fball.h"

#pragma warning(disable:4710)  // 'function': function not inlined (optimzation)

// Setup data

#define CreateComObject(clsid, iid, var) CoCreateInstance( clsid, NULL, CLSCTX_INPROC_SERVER, iid, (void **)&var);
STDAPI AMovieSetupRegisterServer( CLSID   clsServer, LPCWSTR szDescription, LPCWSTR szFileName, LPCWSTR szThreadingModel = L"Both", LPCWSTR szServerType     = L"InprocServer32" );
STDAPI AMovieSetupUnregisterServer( CLSID clsServer );

const AMOVIESETUP_MEDIATYPE sudOpPinTypes =
{
    &MEDIATYPE_Video,       // Major type
    &MEDIASUBTYPE_NULL      // Minor type
};

const AMOVIESETUP_PIN sudOpPin =
{
    L"Output",              // Pin string name
    FALSE,                  // Is it rendered
    TRUE,                   // Is it an output
    FALSE,                  // Can we have none
    FALSE,                  // Can we have many
    &CLSID_NULL,            // Connects to filter
    NULL,                   // Connects to pin
    1,                      // Number of types
    &sudOpPinTypes };       // Pin details

const AMOVIESETUP_FILTER sudBallax =
{
    &CLSID_BouncingBall,    // Filter CLSID
    L"Bouncing Ball",       // String name
    MERIT_DO_NOT_USE,       // Filter merit
    1,                      // Number pins
    &sudOpPin               // Pin details
};


// COM global table of objects in this dll

CFactoryTemplate g_Templates[] = {
  { L"Bouncing Ball"
  , &CLSID_BouncingBall
  , CBouncingBall::CreateInstance
  , NULL
  , &sudBallax }
};
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);


////////////////////////////////////////////////////////////////////////
//
// Exported entry points for registration and unregistration 
// (in this case they only call through to default implementations).
//
////////////////////////////////////////////////////////////////////////

STDAPI RegisterFilters( BOOL bRegister )
{
    HRESULT hr = NOERROR;
    WCHAR achFileName[MAX_PATH];
    char achTemp[MAX_PATH];
    ASSERT(g_hInst != 0);

    if( 0 == GetModuleFileNameA(g_hInst, achTemp, sizeof(achTemp))) 
        return AmHresultFromWin32(GetLastError());

    MultiByteToWideChar(CP_ACP, 0L, achTemp, lstrlenA(achTemp) + 1, 
                       achFileName, NUMELMS(achFileName));
  
    hr = CoInitialize(0);
    if(bRegister)
    {
        hr = AMovieSetupRegisterServer(CLSID_BouncingBall, L"Bouncing Ball", achFileName, L"Both", L"InprocServer32");
    }

    if( SUCCEEDED(hr) )
    {
        IFilterMapper2 *fm = 0;
        hr = CreateComObject( CLSID_FilterMapper2, IID_IFilterMapper2, fm );
        if( SUCCEEDED(hr) )
        {
            if(bRegister)
            {
                IMoniker *pMoniker = 0;
                REGFILTER2 rf2;
                rf2.dwVersion = 1;
                rf2.dwMerit = MERIT_DO_NOT_USE;
                rf2.cPins = 1;
                rf2.rgPins = &sudOpPin;
                hr = fm->RegisterFilter(CLSID_BouncingBall, L"Bouncing Ball", &pMoniker, &CLSID_VideoInputDeviceCategory, NULL, &rf2);
            }
            else
            {
                hr = fm->UnregisterFilter(&CLSID_VideoInputDeviceCategory, 0, CLSID_BouncingBall);
            }
        }

      // release interface
      //
      if(fm)
          fm->Release();
    }

    if( SUCCEEDED(hr) && !bRegister )
        hr = AMovieSetupUnregisterServer( CLSID_BouncingBall );

    CoFreeUnusedLibraries();
    CoUninitialize();
    return hr;
}

//
// DllRegisterServer
//
// Exported entry points for registration and unregistration
//
STDAPI DllRegisterServer()
{
    return RegisterFilters(TRUE);

} // DllRegisterServer


//
// DllUnregisterServer
//
STDAPI DllUnregisterServer()
{
    return RegisterFilters(FALSE);

} // DllUnregisterServer


//
// DllEntryPoint
//
extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

BOOL APIENTRY DllMain(HANDLE hModule, 
                      DWORD  dwReason, 
                      LPVOID lpReserved)
{
	return DllEntryPoint((HINSTANCE)(hModule), dwReason, lpReserved);
}

//
// CreateInstance
//
// The only allowed way to create Bouncing balls!
//
CUnknown * WINAPI CBouncingBall::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr)
{
    ASSERT(phr);
    CUnknown *punk = new CBouncingBall(lpunk, phr);
    return punk;

} // CreateInstance

HRESULT CBouncingBall::QueryInterface(REFIID riid, void **ppv)
{
    //Forward request for IAMStreamConfig & IKsPropertySet to the pin
    if(riid == _uuidof(IAMStreamConfig) || riid == _uuidof(IKsPropertySet))
        return m_paStreams[0]->QueryInterface(riid, ppv);
    else
        return CSource::QueryInterface(riid, ppv);
}

//
// Constructor
//
// Initialise a CBallStream object so that we have a pin.
//
CBouncingBall::CBouncingBall(LPUNKNOWN lpunk, HRESULT *phr) :
    CSource(NAME("Bouncing ball"), lpunk, CLSID_BouncingBall)
{

	ASSERT(phr);
    CAutoLock cAutoLock(&m_cStateLock);
    // Create the one and only output pin
    m_paStreams = (CSourceStream **) new CBallStream*[1];
    m_paStreams[0] = new CBallStream(phr, this, L"Bouncing Ball");

} // (Constructor)

//
// Constructor
//
CBallStream::CBallStream(HRESULT *phr,
                         CBouncingBall *pParent,
                         LPCWSTR pPinName) :
    CSourceStream(NAME("Bouncing Ball"),phr, pParent, pPinName), 
		m_pParent(pParent)
{
    GetMediaType(4, &m_mt);

	memset(&this->rxo, 0x00, sizeof(OVERLAPPED));
	memset(&this->txo, 0x00, sizeof(OVERLAPPED));
	this->pipeHandle = INVALID_HANDLE_VALUE;

	this->currentFrame = NULL;
	this->currentFrameLen = 0;
	this->testCursor = 0;
	this->tmpBuff = NULL;

	this->rxBuff = NULL;
    this->rxBuffLen = 0;
    this->rxBuffAlloc = 0;

	SYSTEMTIME systime;
	GetSystemTime(&systime);
	SystemTimeToFileTime(&systime, &this->lastUpdateTime);

} // (Constructor)

//
// Destructor
//
CBallStream::~CBallStream()
{
	if(this->pipeHandle != 0)
		CloseHandle(this->pipeHandle);
	if(this->currentFrame != NULL)
		delete [] this->currentFrame;
	this->currentFrame = NULL;
	this->currentFrameLen = 0;

	this->pipeHandle = 0;

	if(this->rxBuff!=NULL)
		delete [] this->rxBuff;
	this->rxBuffLen = 0;
	this->rxBuffAlloc = 0;
	if(this->tmpBuff!=NULL)
		delete [] tmpBuff;

} // (Destructor)

HRESULT CBallStream::QueryInterface(REFIID riid, void **ppv)
{   
    // Standard OLE stuff
    if(riid == _uuidof(IAMStreamConfig))
        *ppv = (IAMStreamConfig*)this;
    else if(riid == _uuidof(IKsPropertySet))
        *ppv = (IKsPropertySet*)this;
    else
        return CSourceStream::QueryInterface(riid, ppv);

    AddRef();
    return S_OK;
}

void CBallStream::UpdateNamedPipe()
{
	/*if(this->currentFrame!=NULL)
	{
		delete [] this->currentFrame;
		this->currentFrame = NULL;
		this->currentFrameLen = 0;
	}

	this->currentFrame = NULL;*/

	if(this->pipeHandle == INVALID_HANDLE_VALUE)
	{
		LPCTSTR n = L"\\\\.\\pipe\\testpipe";

		this->pipeHandle = CreateFile(n,
			GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
			NULL);
	}

	if(this->pipeHandle == INVALID_HANDLE_VALUE)
	{
	for(DWORD i=0; i<this->currentFrameLen; i++)
	{
		if(i%3==1)
			this->currentFrame[i] = 0x255;
		else
			this->currentFrame[i] = 0x0;
	}
	}

	if(this->pipeHandle != INVALID_HANDLE_VALUE)
	{
		/*for(DWORD i=0; i<this->currentFrameLen; i++)
		{
			this->currentFrame[i] = 0x255;
		}*/


		//Transmit test message using named pipe
		/*DWORD bytesWritten = 0;
		char test[] = "Test Message";

		if(HasOverlappedIoCompleted(&this->txo))
		{
			BOOL res = WriteFileEx(this->pipeHandle, test, strlen(test), &this->txo, NULL);
		}

		BOOL res = GetOverlappedResult(this->pipeHandle, &txo, &bytesWritten, TRUE);*/

		//Receive messages from named pipe
		const int tmpBuffLen = 1024*1024;
		if(tmpBuff==NULL)
			tmpBuff = new char[tmpBuffLen];
		DWORD bytesRead = 0;
		BOOL res = 0;
		
		if(HasOverlappedIoCompleted(&this->rxo))
		{
			res = ReadFileEx(this->pipeHandle,
			  tmpBuff,
			  tmpBuffLen,
			  &rxo,
			  NULL);
		}

		res = GetOverlappedResult(this->pipeHandle, &this->rxo, &bytesRead, FALSE);

		if(this->currentFrame!=NULL)
		for(DWORD i=0; i<this->currentFrameLen; i++)
		{
			if(i%3==0)
				this->currentFrame[i] = 0xff;
			else
				this->currentFrame[i] = 0x0;
		}

		if(res && bytesRead > 0)
		{
			//Merge receive string with buffer
			/*if(rxBuff != NULL && rxBuffLen + bytesRead <= rxBuffAlloc)
			{
				//No need to reallocate
				memcpy(&rxBuff[rxBuffLen], buff, bytesRead);
				rxBuffLen += bytesRead;
			}
			else
			{
				//Buffer must be resized
				if(rxBuff != NULL)
				{
						char *tmp = new (std::nothrow) char[rxBuffLen + bytesRead];
						if(tmp!=NULL)
						{
							memcpy(tmp, rxBuff, rxBuffLen);
							memcpy(&tmp[rxBuffLen], buff, bytesRead);
							delete [] rxBuff;

							rxBuff = tmp;
							rxBuffLen = rxBuffLen + bytesRead;
							rxBuffAlloc = rxBuffLen + bytesRead;
						}
						else
						{
							return;
						}
				}
				else
				{
					rxBuff = new (std::nothrow) char[bytesRead];
					if(rxBuff == NULL)
					{
						return;
					}
					memcpy(rxBuff, buff, bytesRead);

					rxBuffLen = bytesRead;
					rxBuffAlloc = bytesRead;
				}
			}*/

			if(this->currentFrame!=NULL)
			for(DWORD i=0; i<this->currentFrameLen; i++)
			{
				if(i%3==2)
					this->currentFrame[i] = 0xff;
				else
					this->currentFrame[i] = 0x00;
			}

			/*UINT32 cursor = 0;
			int processing = 1;
			while(processing && (rxBuffLen - cursor) > 8 && rxBuff != NULL)
			{
				UINT32 *wordArray = (UINT32 *)&rxBuff[cursor];
				UINT32 msgType = wordArray[0];
				UINT32 msgLen = wordArray[1];
				if(rxBuffLen-cursor >= 8+msgLen)
				{
					char *payload = &this->rxBuff[cursor+8];
					UINT32 payloadLen = msgLen - 8;
					UINT32 *payloadArray = (UINT32 *)payload;

					if(msgType == 2)
					{
						//Message is new frame
						for(unsigned i=0; i<payloadLen && i<this->currentFrameLen; i++)
						{
							this->currentFrame[i] = payload[i];
						}
					}

					cursor += 8+msgLen;
				}
				else
				{
					processing = 0;
				}
			}
			*/
			//Store unprocessed data in buffer
			/*if(cursor > 0 && rxBuff != NULL)
			{
				char *tmp = new (std::nothrow) char[rxBuffLen - cursor];
				if(tmp==NULL)
				{
					rxBuffLen = 0;
					return;
				}
				memcpy(tmp, &rxBuff[cursor], rxBuffLen - cursor);
				delete [] rxBuff;
				rxBuff = tmp;

				rxBuffAlloc = rxBuffLen - cursor;
				rxBuffLen = rxBuffLen - cursor;
			}*/
			rxBuffLen = 0;
			
		}
	}

	/*if(this->currentFrame != NULL)
	{
		for(DWORD i=0; i<currentFrameLen; i++)
		{
			this->currentFrame[i] = rand();
		}
	}*/
	
	
}


void CBallStream::SendStatusViaNamedPipe(UINT32 width, UINT32 height, UINT32 bufflen)
{
	if(this->pipeHandle != INVALID_HANDLE_VALUE)
	{
		/*for(DWORD i=0; i<this->currentFrameLen; i++)
		{
			this->currentFrame[i] = 0x255;
		}*/

		//Transmit test message using named pipe
		DWORD bytesWritten = 0;
		const int buffLen = 4*5;
		char test[buffLen];
		UINT32 *pMsgType = (UINT32 *)&test[0];
		*pMsgType = 1;
		UINT32 *pMsgLen = (UINT32 *)&test[4];
		*pMsgLen = 4*3;
		UINT32 *pWidth = (UINT32 *)&test[8];
		*pWidth = width;
		UINT32 *pHeight = (UINT32 *)&test[12];
		*pHeight = height;
		UINT32 *pBuffLen = (UINT32 *)&test[16];
		*pBuffLen = bufflen;

		if(HasOverlappedIoCompleted(&this->txo))
		{
			BOOL res = WriteFileEx(this->pipeHandle, test, buffLen, &this->txo, NULL);
		}

		BOOL res = GetOverlappedResult(this->pipeHandle, &txo, &bytesWritten, TRUE);
	}

}

void CBallStream::SendErrorViaNamedPipe(UINT32 errCode)
{
	if(this->pipeHandle != INVALID_HANDLE_VALUE)
	{
		/*for(DWORD i=0; i<this->currentFrameLen; i++)
		{
			this->currentFrame[i] = 0x255;
		}*/

		//Transmit test message using named pipe
		DWORD bytesWritten = 0;
		const int buffLen = 4*3;
		char test[buffLen];
		UINT32 *pMsgType = (UINT32 *)&test[0];
		*pMsgType = 1;
		UINT32 *pMsgLen = (UINT32 *)&test[4];
		*pMsgLen = 4;
		UINT32 *pError = (UINT32 *)&test[8];
		*pError = errCode;

		if(HasOverlappedIoCompleted(&this->txo))
		{
			BOOL res = WriteFileEx(this->pipeHandle, test, buffLen, &this->txo, NULL);
		}

		BOOL res = GetOverlappedResult(this->pipeHandle, &txo, &bytesWritten, TRUE);
	}

}


//
// FillBuffer
//
// Plots a ball into the supplied video buffer
//
HRESULT CBallStream::FillBuffer(IMediaSample *pms)
{
    REFERENCE_TIME rtNow;
    
	VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *)(m_mt.Format());
    REFERENCE_TIME avgFrameTime = ((VIDEOINFOHEADER*)m_mt.pbFormat)->AvgTimePerFrame;

	LONG width = pvi->bmiHeader.biWidth;
	LONG height = pvi->bmiHeader.biHeight;

    rtNow = m_rtLastTime;
    m_rtLastTime += avgFrameTime;
    pms->SetTime(&rtNow, &m_rtLastTime);
    pms->SetSyncPoint(TRUE);

    BYTE *pData;
    long lDataLen;
    pms->GetPointer(&pData);
    lDataLen = pms->GetSize();

	/*if(this->currentFrame != NULL)
	{
		delete [] this->currentFrame;
		this->currentFrame = NULL;
		this->currentFrameLen = 0;
	}*/

	//Calculate time since last frame update
	SYSTEMTIME systime;
	GetSystemTime(&systime);
	FILETIME fiTime;
	SystemTimeToFileTime(&systime, &fiTime);
	LARGE_INTEGER fiTimeNum;
	fiTimeNum.HighPart = fiTime.dwHighDateTime;
	fiTimeNum.LowPart = fiTime.dwLowDateTime;
	LARGE_INTEGER lastUpdate;
	lastUpdate.HighPart = lastUpdateTime.dwHighDateTime;
	lastUpdate.LowPart = lastUpdateTime.dwLowDateTime;

	LARGE_INTEGER elapse;
	elapse.QuadPart = fiTimeNum.QuadPart - lastUpdate.QuadPart;
	float elapseMs = elapse.LowPart / 10000.f;

	if(elapseMs > 100.)
	{
		this->UpdateNamedPipe();
		this->SendStatusViaNamedPipe(width, height, lDataLen);

		if(this->currentFrame == NULL)
		{
			this->currentFrame = new BYTE[lDataLen];
			this->currentFrameLen = lDataLen;
	
			long cursor = 0;
			for(LONG y=0; y < height; y++)
			for(LONG x=0; x < width; x++)
			{
				if(cursor > this->currentFrameLen) continue;

				this->currentFrame[cursor] = x % 255; //Blue
				this->currentFrame[cursor+1] = y % 255; //Green
				this->currentFrame[cursor+2] = rand(); //Red 

				cursor += 3;
			}
		}

		if(this->currentFrame != NULL)
			memcpy(pData, this->currentFrame, lDataLen);

		lastUpdateTime=fiTime;
	}

	/*for(LONG i=0;i<lDataLen;i++)
	{
		pData[i] = rand();
	}*/

    return NOERROR;

} // FillBuffer


//
// Notify
//
// Alter the repeat rate according to quality management messages sent from
// the downstream filter (often the renderer).  Wind it up or down according
// to the flooding level - also skip forward if we are notified of Late-ness
//
STDMETHODIMP CBallStream::Notify(IBaseFilter * pSender, Quality q)
{
    return E_NOTIMPL;

} // Notify


//
// SetMediaType
//
// Called when a media type is agreed between filters
//
HRESULT CBallStream::SetMediaType(const CMediaType *pMediaType)
{
    DECLARE_PTR(VIDEOINFOHEADER, pvi, pMediaType->Format());
    HRESULT hr = CSourceStream::SetMediaType(pMediaType);
    return hr;
} // SetMediaType

//////////////////////////////////////////////////////////////////////////
// This is called when the output format has been negotiated
//////////////////////////////////////////////////////////////////////////

// See Directshow help topic for IAMStreamConfig for details on this method
HRESULT CBallStream::GetMediaType(int iPosition, CMediaType *pmt)
{
    if(iPosition < 0) return E_INVALIDARG;
    if(iPosition > 8) return VFW_S_NO_MORE_ITEMS;

    if(iPosition == 0) 
    {
        *pmt = m_mt;
        return S_OK;
    }

    DECLARE_PTR(VIDEOINFOHEADER, pvi, pmt->AllocFormatBuffer(sizeof(VIDEOINFOHEADER)));
    ZeroMemory(pvi, sizeof(VIDEOINFOHEADER));

    pvi->bmiHeader.biCompression = BI_RGB;
    pvi->bmiHeader.biBitCount    = 24;
    pvi->bmiHeader.biSize       = sizeof(BITMAPINFOHEADER);
    pvi->bmiHeader.biWidth      = 80 * iPosition;
    pvi->bmiHeader.biHeight     = 60 * iPosition;
    pvi->bmiHeader.biPlanes     = 1;
    pvi->bmiHeader.biSizeImage  = GetBitmapSize(&pvi->bmiHeader);
    pvi->bmiHeader.biClrImportant = 0;

    pvi->AvgTimePerFrame = 1000000;

    SetRectEmpty(&(pvi->rcSource)); // we want the whole image area rendered.
    SetRectEmpty(&(pvi->rcTarget)); // no particular destination rectangle

    pmt->SetType(&MEDIATYPE_Video);
    pmt->SetFormatType(&FORMAT_VideoInfo);
    pmt->SetTemporalCompression(FALSE);

    // Work out the GUID for the subtype from the header info.
    const GUID SubTypeGUID = GetBitmapSubtype(&pvi->bmiHeader);
    pmt->SetSubtype(&SubTypeGUID);
    pmt->SetSampleSize(pvi->bmiHeader.biSizeImage);
    
    return NOERROR;

} // GetMediaType

// This method is called to see if a given output format is supported
HRESULT CBallStream::CheckMediaType(const CMediaType *pMediaType)
{
    VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *)(pMediaType->Format());
    if(*pMediaType != m_mt) 
        return E_INVALIDARG;
    return S_OK;
} // CheckMediaType

// This method is called after the pins are connected to allocate buffers to stream data
HRESULT CBallStream::DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProperties)
{
    CAutoLock cAutoLock(m_pFilter->pStateLock());
    HRESULT hr = NOERROR;

    VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *) m_mt.Format();
    pProperties->cBuffers = 1;
    pProperties->cbBuffer = pvi->bmiHeader.biSizeImage;

    ALLOCATOR_PROPERTIES Actual;
    hr = pAlloc->SetProperties(pProperties,&Actual);

    if(FAILED(hr)) return hr;
    if(Actual.cbBuffer < pProperties->cbBuffer) return E_FAIL;

    return NOERROR;
} // DecideBufferSize

// Called when graph is run
HRESULT CBallStream::OnThreadCreate()
{
	//m_iRepeatTime = m_iDefaultRepeatTime;
    m_rtLastTime = 0;
    return NOERROR;
} // OnThreadCreate


//////////////////////////////////////////////////////////////////////////
//  IAMStreamConfig
//////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE CBallStream::SetFormat(AM_MEDIA_TYPE *pmt)
{
    DECLARE_PTR(VIDEOINFOHEADER, pvi, m_mt.pbFormat);
    m_mt = *pmt;
    IPin* pin; 
    ConnectedTo(&pin);
    if(pin)
    {
        IFilterGraph *pGraph = m_pParent->GetGraph();
        pGraph->Reconnect(this);
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CBallStream::GetFormat(AM_MEDIA_TYPE **ppmt)
{
    *ppmt = CreateMediaType(&m_mt);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CBallStream::GetNumberOfCapabilities(int *piCount, int *piSize)
{
    *piCount = 8;
    *piSize = sizeof(VIDEO_STREAM_CONFIG_CAPS);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CBallStream::GetStreamCaps(int iIndex, AM_MEDIA_TYPE **pmt, BYTE *pSCC)
{
    *pmt = CreateMediaType(&m_mt);
    DECLARE_PTR(VIDEOINFOHEADER, pvi, (*pmt)->pbFormat);

    if (iIndex == 0) iIndex = 4;

    pvi->bmiHeader.biCompression = BI_RGB;
    pvi->bmiHeader.biBitCount    = 24;
    pvi->bmiHeader.biSize       = sizeof(BITMAPINFOHEADER);
    pvi->bmiHeader.biWidth      = 80 * iIndex;
    pvi->bmiHeader.biHeight     = 60 * iIndex;
    pvi->bmiHeader.biPlanes     = 1;
    pvi->bmiHeader.biSizeImage  = GetBitmapSize(&pvi->bmiHeader);
    pvi->bmiHeader.biClrImportant = 0;

    SetRectEmpty(&(pvi->rcSource)); // we want the whole image area rendered.
    SetRectEmpty(&(pvi->rcTarget)); // no particular destination rectangle

    (*pmt)->majortype = MEDIATYPE_Video;
    (*pmt)->subtype = MEDIASUBTYPE_RGB24;
    (*pmt)->formattype = FORMAT_VideoInfo;
    (*pmt)->bTemporalCompression = FALSE;
    (*pmt)->bFixedSizeSamples= FALSE;
    (*pmt)->lSampleSize = pvi->bmiHeader.biSizeImage;
    (*pmt)->cbFormat = sizeof(VIDEOINFOHEADER);
    
    DECLARE_PTR(VIDEO_STREAM_CONFIG_CAPS, pvscc, pSCC);
    
    pvscc->guid = FORMAT_VideoInfo;
    pvscc->VideoStandard = AnalogVideo_None;
    pvscc->InputSize.cx = 640;
    pvscc->InputSize.cy = 480;
    pvscc->MinCroppingSize.cx = 80;
    pvscc->MinCroppingSize.cy = 60;
    pvscc->MaxCroppingSize.cx = 640;
    pvscc->MaxCroppingSize.cy = 480;
    pvscc->CropGranularityX = 80;
    pvscc->CropGranularityY = 60;
    pvscc->CropAlignX = 0;
    pvscc->CropAlignY = 0;

    pvscc->MinOutputSize.cx = 80;
    pvscc->MinOutputSize.cy = 60;
    pvscc->MaxOutputSize.cx = 640;
    pvscc->MaxOutputSize.cy = 480;
    pvscc->OutputGranularityX = 0;
    pvscc->OutputGranularityY = 0;
    pvscc->StretchTapsX = 0;
    pvscc->StretchTapsY = 0;
    pvscc->ShrinkTapsX = 0;
    pvscc->ShrinkTapsY = 0;
    pvscc->MinFrameInterval = 200000;   //50 fps
    pvscc->MaxFrameInterval = 50000000; // 0.2 fps
    pvscc->MinBitsPerSecond = (80 * 60 * 3 * 8) / 5;
    pvscc->MaxBitsPerSecond = 640 * 480 * 3 * 8 * 50;

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////
// IKsPropertySet
//////////////////////////////////////////////////////////////////////////


HRESULT CBallStream::Set(REFGUID guidPropSet, DWORD dwID, void *pInstanceData, 
                        DWORD cbInstanceData, void *pPropData, DWORD cbPropData)
{// Set: Cannot set any properties.
    return E_NOTIMPL;
}

// Get: Return the pin category (our only property). 
HRESULT CBallStream::Get(
    REFGUID guidPropSet,   // Which property set.
    DWORD dwPropID,        // Which property in that set.
    void *pInstanceData,   // Instance data (ignore).
    DWORD cbInstanceData,  // Size of the instance data (ignore).
    void *pPropData,       // Buffer to receive the property data.
    DWORD cbPropData,      // Size of the buffer.
    DWORD *pcbReturned     // Return the size of the property.
)
{
    if (guidPropSet == AMPROPSETID_Pin)
	{
		if (dwPropID != AMPROPERTY_PIN_CATEGORY)        return E_PROP_ID_UNSUPPORTED;
		if (pPropData == NULL && pcbReturned == NULL)   return E_POINTER;
    
		if (pcbReturned) *pcbReturned = sizeof(GUID);
		if (pPropData == NULL)          return S_OK; // Caller just wants to know the size. 
		if (cbPropData < sizeof(GUID))  return E_UNEXPECTED;// The buffer is too small.
        
		*(GUID *)pPropData = PIN_CATEGORY_CAPTURE;
		return S_OK;
	}



	return E_PROP_SET_UNSUPPORTED;
}

// QuerySupported: Query whether the pin supports the specified property.
HRESULT CBallStream::QuerySupported(REFGUID guidPropSet, DWORD dwPropID, DWORD *pTypeSupport)
{
    if (guidPropSet != AMPROPSETID_Pin) return E_PROP_SET_UNSUPPORTED;
    if (dwPropID != AMPROPERTY_PIN_CATEGORY) return E_PROP_ID_UNSUPPORTED;
    // We support getting this property, but not setting it.
    if (pTypeSupport) *pTypeSupport = KSPROPERTY_SUPPORT_GET; 
    return S_OK;
}

DWORD CBallStream::ThreadProc()
{
	return CSourceStream::ThreadProc();
}