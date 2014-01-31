// {3A24BD2F-B9B1-4B32-9A1E-17791624B6AB}
DEFINE_GUID(CLSID_Kinatomic_Camera,
0x3a24bd2f, 0xb9b1, 0x4b32, 0x9a, 0x1e, 0x17, 0x79, 0x16, 0x24, 0xb6, 0xab);

#define DECLARE_PTR(type, ptr, expr) type* ptr = (type*)(expr);

//------------------------------------------------------------------------------
// Forward Declarations
//------------------------------------------------------------------------------
// The class managing the output pin
class CCameraStream;


//------------------------------------------------------------------------------
// Class CCameraOutput
//
// This is the main class for the camera output. It inherits from
// CSource, the DirectShow base class for source filters.
//------------------------------------------------------------------------------
class CCameraOutput : public CSource
{
public:

    // The only allowed way to create Bouncing balls!
    static CUnknown * WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);

	STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
	IFilterGraph *GetGraph() {return m_pGraph;}
private:

    // It is only allowed to to create these objects with CreateInstance
    CCameraOutput(LPUNKNOWN lpunk, HRESULT *phr);

}; // CBouncingBall


//------------------------------------------------------------------------------
// Class CCameraStream
//
// This class implements the stream which is used to output the bouncing ball
// data from the source filter. It inherits from DirectShows's base
// CSourceStream class.
//------------------------------------------------------------------------------
class CCameraStream : public CSourceStream, public IAMStreamConfig, public IKsPropertySet
{

public:

    CCameraStream(HRESULT *phr, CCameraOutput *pParent, LPCWSTR pPinName);
    ~CCameraStream();

    // Update image buffer
    HRESULT FillBuffer(IMediaSample *pms);

    // Ask for buffers of the size appropriate to the agreed media type
    HRESULT DecideBufferSize(IMemAllocator *pIMemAlloc,
                             ALLOCATOR_PROPERTIES *pProperties);

    // Set the agreed media type
    HRESULT SetMediaType(const CMediaType *pMediaType);

    HRESULT CheckMediaType(const CMediaType *pMediaType);
    HRESULT GetMediaType(int iPosition, CMediaType *pmt);

    // Resets the stream time to zero
    HRESULT OnThreadCreate(void);

    // Quality control notifications sent to us
    STDMETHODIMP Notify(IBaseFilter * pSender, Quality q);

	DWORD ThreadProc();
	int EstablishPipeConnection();
	int ReceiveDataViaNamedPipe();
	void SendStatusViaNamedPipe(UINT32 width, UINT32 height, UINT32 bufflen);
	void SendErrorViaNamedPipe(UINT32 errCode);

	STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
	STDMETHODIMP_(ULONG) AddRef() { return GetOwner()->AddRef(); }                                                          \
    STDMETHODIMP_(ULONG) Release() { return GetOwner()->Release(); }

	HRESULT STDMETHODCALLTYPE SetFormat(AM_MEDIA_TYPE *pmt);
    HRESULT STDMETHODCALLTYPE GetFormat(AM_MEDIA_TYPE **ppmt);
    HRESULT STDMETHODCALLTYPE GetNumberOfCapabilities(int *piCount, int *piSize);
    HRESULT STDMETHODCALLTYPE GetStreamCaps(int iIndex, AM_MEDIA_TYPE **pmt, BYTE *pSCC);

	HRESULT STDMETHODCALLTYPE Set(REFGUID guidPropSet, DWORD dwID, void *pInstanceData, DWORD cbInstanceData, void *pPropData, DWORD cbPropData);
    HRESULT STDMETHODCALLTYPE Get(REFGUID guidPropSet, DWORD dwPropID, void *pInstanceData,DWORD cbInstanceData, void *pPropData, DWORD cbPropData, DWORD *pcbReturned);
    HRESULT STDMETHODCALLTYPE QuerySupported(REFGUID guidPropSet, DWORD dwPropID, DWORD *pTypeSupport);

private:

    //int m_iImageHeight;                 // The current image height
    //int m_iImageWidth;                  // And current image width
    //int m_iRepeatTime;                  // Time in msec between frames
    //const int m_iDefaultRepeatTime;     // Initial m_iRepeatTime

    //BYTE m_BallPixel[4];                // Represents one coloured ball
    //int m_iPixelSize;                   // The pixel size in bytes
    //PALETTEENTRY m_Palette[256];        // The optimal palette for the image

    CCritSec m_cSharedState;            // Lock on m_rtSampleTime and m_Ball
    CRefTime m_rtSampleTime;            // The time stamp for each sample
    //CBall *m_Ball;                      // The current ball object
	CCameraOutput *m_pParent;

	char *rxBuff;
	int rxBuffLen;
	int rxBuffAlloc;

	REFERENCE_TIME m_rtLastTime;

	HANDLE pipeHandle;
	OVERLAPPED rxo;
	OVERLAPPED txo;

	BYTE *currentFrame;
	LONG currentFrameLen;
	int testCursor;
	char *tmpBuff;
	int fillBufferCount;

	FILETIME lastTxUpdateTime;
	FILETIME lastRxUpdateTime;

    // set up the palette appropriately
    //enum Colour {Red, Blue, Green, Yellow};
    //HRESULT SetPaletteEntries(Colour colour);

}; // CCameraStream
    

