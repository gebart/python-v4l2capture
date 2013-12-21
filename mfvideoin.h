
#ifndef MFVIDEOIN_H
#define MFVIDEOIN_H

#include <vector>
#include <string>
#include <mfidl.h>
#include <mfreadwrite.h>
#include "base.h"

class WmfBase : public Base_Video_In
{
public:
	WmfBase();
	virtual ~WmfBase();

};

class MfVideoIn : public WmfBase
{
public:
	MfVideoIn(const wchar_t *devName);
	virtual ~MfVideoIn();

	virtual void Stop();
	virtual void WaitForStop();
	virtual void OpenDevice();
	virtual void SetFormat(const char *fmt, int width, int height);
	virtual void StartDevice(int buffer_count);
	virtual void StopDevice();
	virtual void CloseDevice();
	virtual int GetFrame(unsigned char **buffOut, class FrameMetaData *metaOut);

	virtual int GetMfParameter(long prop = 0);

	void Run();
protected:
	
	IMFSourceReader* reader;
	IMFMediaSource* source;
	int asyncMode;
	std::wstring devName;
	class SourceReaderCB* readerCallback;
	int stopping;
	int stopped;
	int openDevFlag;
	int startDevFlag;
	int stopDevFlag;
	int closeDevFlag;
	CRITICAL_SECTION lock;
	unsigned maxBuffSize;

	std::vector<char *> frameBuff;
	std::vector<DWORD> frameLenBuff;
	std::vector<HRESULT> hrStatusBuff;
	std::vector<DWORD> dwStreamIndexBuff;
    std::vector<DWORD> dwStreamFlagsBuff;
	std::vector<LONGLONG> llTimestampBuff;

	std::vector<LONG> plStrideBuff;
	std::vector<std::wstring> majorTypeBuff, subTypeBuff;
	std::vector<UINT32> widthBuff;
	std::vector<UINT32> heightBuff;
	std::vector<char> isCompressedBuff;

	void OpenDeviceInternal();
	void StartDeviceInternal();
	void SetSampleMetaData(DWORD streamIndex);
	void PopFrontMetaDataBuff();
	void ReadFramesInternal();
	void StopDeviceInternal();
	void CloseDeviceInternal();
};

void *MfVideoIn_Worker_thread(void *arg);
std::vector<std::vector<std::wstring> > List_in_devices();

#endif //MFVIDEOIN_H
