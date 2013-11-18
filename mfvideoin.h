
#ifndef MFVIDEOIN_H
#define MFVIDEOIN_H

#include <vector>
#include <string>
#include <mfidl.h>
#include <mfreadwrite.h>
#include "base.h"

class MfVideoIn : public Base_Video_In
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

protected:
	virtual void InitWmf();
	virtual void DeinitWmf();

	int initDone;
	IMFSourceReader* reader;
	IMFMediaSource* source;
	int asyncMode;
	std::string devName;
	class SourceReaderCB* readerCallback;
};

void *MfVideoIn_Worker_thread(void *arg);
std::vector<std::string> List_in_devices();

#endif //MFVIDEOIN_H
