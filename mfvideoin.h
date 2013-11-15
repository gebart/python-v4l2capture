
#ifndef MFVIDEOIN_H
#define MFVIDEOIN_H

#include "base.h"

class MfVideoIn : public Base_Video_In
{
public:
	MfVideoIn() : Base_Video_In() {};
	virtual ~MfVideoIn() {};

	virtual void Stop() {};
	virtual void WaitForStop() {};
	virtual void OpenDevice() {};
	virtual void SetFormat(const char *fmt, int width, int height) {};
	virtual void StartDevice(int buffer_count) {};
	virtual void StopDevice() {};
	virtual void CloseDevice() {};
	virtual int GetFrame(unsigned char **buffOut, class FrameMetaData *metaOut) {return 0;};
};

#endif //MFVIDEOIN_H
