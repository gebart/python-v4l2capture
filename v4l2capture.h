// python-v4l2capture
// Python extension to capture video with video4linux2
//
// 2009, 2010, 2011 Fredrik Portstrom, released into the public domain
// 2011, Joakim Gebart
// 2013, Tim Sheerman-Chase
// See README for license

#ifndef V4L2CAPTURE_H
#define V4L2CAPTURE_H

#include <vector>
#include <pthread.h>
#include <string>

struct buffer {
	void *start;
	size_t length;
};

struct capability {
	int id;
	const char *name;
};


class FrameMetaData
{
public:
	std::string fmt;
	int width;
	int height;
	unsigned buffLen;
	unsigned long sequence;
	unsigned long tv_sec;
	unsigned long tv_usec;

	FrameMetaData()
	{
		width = 0;
		height = 0;
		buffLen = 0;
		sequence = 0;
		tv_sec = 0;
		tv_usec = 0;
	}

	FrameMetaData(const FrameMetaData &in)
	{
		FrameMetaData::operator=(in);
	}	

	const FrameMetaData &operator=(const FrameMetaData &in)
	{
		width = in.width;
		height = in.height;
		fmt = in.fmt;
		buffLen = in.buffLen;
		sequence = in.sequence;
		tv_sec = in.tv_sec;
		tv_usec = in.tv_usec;
		return *this;
	}

};

// **********************************************************************

class SetFormatParams
{
public:
	std::string fmt;
	int width, height;

	SetFormatParams()
	{
		width = 0;
		height = 0;
	}

	SetFormatParams(const SetFormatParams &in)
	{
		SetFormatParams::operator=(in);
	}	

	const SetFormatParams &operator=(const SetFormatParams &in)
	{
		width = in.width;
		height = in.height;
		fmt = in.fmt;
		return *this;
	}
};

/*static struct capability capabilities[] = {
	{ V4L2_CAP_ASYNCIO, "asyncio" },
	{ V4L2_CAP_AUDIO, "audio" },
	{ V4L2_CAP_HW_FREQ_SEEK, "hw_freq_seek" },
	{ V4L2_CAP_RADIO, "radio" },
	{ V4L2_CAP_RDS_CAPTURE, "rds_capture" },
	{ V4L2_CAP_READWRITE, "readwrite" },
	{ V4L2_CAP_SLICED_VBI_CAPTURE, "sliced_vbi_capture" },
	{ V4L2_CAP_SLICED_VBI_OUTPUT, "sliced_vbi_output" },
	{ V4L2_CAP_STREAMING, "streaming" },
	{ V4L2_CAP_TUNER, "tuner" },
	{ V4L2_CAP_VBI_CAPTURE, "vbi_capture" },
	{ V4L2_CAP_VBI_OUTPUT, "vbi_output" },
	{ V4L2_CAP_VIDEO_CAPTURE, "video_capture" },
	{ V4L2_CAP_VIDEO_OUTPUT, "video_output" },
	{ V4L2_CAP_VIDEO_OUTPUT_OVERLAY, "video_output_overlay" },
	{ V4L2_CAP_VIDEO_OVERLAY, "video_overlay" }
};*/

int my_ioctl(int fd, int request, void *arg, int utimeout);

class Video_in_Manager
{
public:
	//Device_manager *self;
	std::string devName;
	int stop;
	int stopped;
	pthread_mutex_t lock;
	std::vector<std::string> openDeviceFlag;
	std::vector<int> startDeviceFlag;
	std::vector<class SetFormatParams> setFormatFlags;
	int stopDeviceFlag;
	int closeDeviceFlag;
	int deviceStarted;
	int fd;
	struct buffer *buffers;
	int frameWidth, frameHeight;
	int buffer_counts;
	std::string pxFmt;
	int verbose;
	std::string targetFmt;

	std::vector<unsigned char *> decodedFrameBuff;
	std::vector<class FrameMetaData> decodedFrameMetaBuff;
	unsigned decodedFrameBuffMaxSize;

	Video_in_Manager(const char *devNameIn);
	virtual ~Video_in_Manager();
	void Stop();
	void WaitForStop();
	void OpenDevice();
	void SetFormat(const char *fmt, int width, int height);
	void StartDevice(int buffer_count);
	void StopDevice();
	void CloseDevice();
	int GetFrame(unsigned char **buffOut, class FrameMetaData *metaOut);

protected:
	int ReadFrame();
	int OpenDeviceInternal();
	int SetFormatInternal(class SetFormatParams &args);
	int GetFormatInternal();
	int StartDeviceInternal(int buffer_count);
	void StopDeviceInternal();
	int CloseDeviceInternal();

public:
	void Run();
};

void *Video_in_Worker_thread(void *arg);

// **********************************************************************

#endif //V4L2CAPTURE_H

