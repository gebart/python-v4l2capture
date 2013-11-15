
#ifndef MFVIDEOOUT_H
#define MFVIDEOOUT_H

#include "base.h"

class MfVideoOut : public Base_Video_Out
{
public:
	MfVideoOut() : Base_Video_Out() {};
	virtual ~MfVideoOut() {};

	void SendFrame(const char *imgIn, unsigned imgLen, const char *pxFmt, int width, int height) {};
	void Stop() {};
	int WaitForStop() {return 1;};
};

#endif //MFVIDEOOUT_H

