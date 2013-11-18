
#ifndef MFVIDEOIN_H
#define MFVIDEOIN_H

#include <vector>
#include <string>
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
};

void *MfVideoIn_Worker_thread(void *arg);
std::vector<std::string> List_in_devices();

#endif //MFVIDEOIN_H
