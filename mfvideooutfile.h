
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

	void SendFrame(const char *imgIn, unsigned imgLen, const char *pxFmt, int width, int height);
	void Stop();
	int WaitForStop();

	virtual void SetOutputSize(int width, int height);
	virtual void SetOutputPxFmt(const char *fmt);

	void Run();


protected:
	IMFSinkWriter   *pSinkWriter;
	DWORD		   streamIndex;	 
	LONGLONG rtStart;
	UINT64 rtDuration;

};

void *MfVideoOut_File_Worker_thread(void *arg);

#endif //MFVIDEOOUT_H

