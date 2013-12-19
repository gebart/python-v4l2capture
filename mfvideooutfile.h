
#ifndef MFVIDEOOUT_H
#define MFVIDEOOUT_H

#include <vector>
#include <string>
#include <Mfidl.h>
#include <Mfreadwrite.h>
#include "base.h"

class MfVideoOutFile : public Base_Video_Out
{
public:
	MfVideoOutFile(const char *devName);
	virtual ~MfVideoOutFile();

	void OpenFile();
	void CloseFile();

	void SendFrame(const char *imgIn, unsigned imgLen, const char *pxFmt, int width, int height, 
		unsigned long tv_sec = 0,
		unsigned long tv_usec = 0);
	void Stop();
	int WaitForStop();

	virtual void SetOutputSize(int width, int height);
	virtual void SetOutputPxFmt(const char *fmt);
	virtual void SetFrameRate(unsigned int frameRateIn);
	virtual void SetVideoCodec(const char *codec, unsigned int bitrate);

	void MfVideoOutFile::CopyFromBufferToOutFile(int lastFrame = 0);
	void Run();

protected:
	IMFSinkWriter *pSinkWriter;
	DWORD streamIndex;	 
	LONGLONG rtStart;
	UINT64 rtDuration;
	std::string pxFmt;
	std::string videoCodec;
	std::wstring fina;

	int outputWidth, outputHeight;
	UINT32 bitRate, frameRateFps;
	FILETIME startVideoTime;
	std::vector<class FrameMetaData> outBufferMeta;
	std::vector<std::string> outBuffer;
	LONGLONG prevFrameDuration;
};

void *MfVideoOut_File_Worker_thread(void *arg);

#endif //MFVIDEOOUT_H

