
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
	MfVideoIn(const char *devName);
	virtual ~MfVideoIn();

	virtual void Stop();
	virtual void WaitForStop();
	virtual void OpenDevice();
	virtual void SetFormat(const char *fmt, int width, int height);
	virtual void StartDevice(int buffer_count);
	virtual void StopDevice();
	virtual void CloseDevice();
	virtual int GetFrame(unsigned char **buffOut, class FrameMetaData *metaOut);

	void Run();
protected:
	
	IMFSourceReader* reader;
	IMFMediaSource* source;
	int asyncMode;
	std::string devName;
	class SourceReaderCB* readerCallback;
	int stopping;
	int stopped;
	int openDevFlag;
	int startDevFlag;
	int stopDevFlag;
	int closeDevFlag;
	CRITICAL_SECTION lock;
};

void *MfVideoIn_Worker_thread(void *arg);
std::vector<std::vector<std::wstring> > List_in_devices();

#endif //MFVIDEOIN_H
