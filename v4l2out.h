#ifndef __V4L2OUT_H__
#define __V4L2OUT_H__

#include <map>
#include <vector>
#include <string>
#include "base.h"

class Video_out : public Base_Video_Out
{
public:
	std::string devName;
	int stop;
	int stopped;
	pthread_mutex_t lock;
	int verbose;
	std::vector<class SendFrameArgs> sendFrameArgs;
	std::vector<const char *> sendFrameBuffer;
	struct timespec lastFrameTime;
	int fdwr;
	int framesize;
	unsigned char *currentFrame;
	int outputWidth;
	int outputHeight;
	std::string outputPxFmt;

	Video_out(const char *devNameIn);
	virtual ~Video_out();

protected:
	void SendFrameInternal();

public:
	void Run();
	void SendFrame(const char *imgIn, unsigned imgLen, const char *pxFmt, int width, int height);
	void Stop();
	int WaitForStop();

	void SetOutputSize(int width, int height);
	void SetOutputPxFmt(const char *fmt);
};

void *Video_out_manager_Worker_thread(void *arg);

std::vector<std::string> List_out_devices();

// ******************************************************************

#endif //__V4L2OUT_H__


