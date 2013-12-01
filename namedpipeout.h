
#ifndef MFVIDEOOUT_H
#define MFVIDEOOUT_H

#include <vector>
#include <string>
#include "base.h"

class NamedPipeOut : public Base_Video_Out
{
public:
	NamedPipeOut(const char *devName);
	virtual ~NamedPipeOut();

	void SendFrame(const char *imgIn, unsigned imgLen, const char *pxFmt, int width, int height);
	void Stop();
	int WaitForStop();

	virtual void SetOutputSize(int width, int height);
	virtual void SetOutputPxFmt(const char *fmt);

	void Run();

};

void *NamedPipeOut_Worker_thread(void *arg);

std::vector<std::string> List_out_devices();

#endif //MFVIDEOOUT_H

