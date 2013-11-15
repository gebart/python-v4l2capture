
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include <signal.h>
#include <time.h>
#include <dirent.h>
#include <vector>
#include <errno.h>
#include <dirent.h>
#include <stdexcept>
#include "v4l2out.h"
#include "pixfmt.h"

#define ROUND_UP_2(num)  (((num)+1)&~1)
#define ROUND_UP_4(num)  (((num)+3)&~3)
#define ROUND_UP_8(num)  (((num)+7)&~7)

void print_format(struct v4l2_format*vid_format) {
  printf("	vid_format->type                =%d\n",	vid_format->type );
  printf("	vid_format->fmt.pix.width       =%d\n",	vid_format->fmt.pix.width );
  printf("	vid_format->fmt.pix.height      =%d\n",	vid_format->fmt.pix.height );
  printf("	vid_format->fmt.pix.pixelformat =%d\n",	vid_format->fmt.pix.pixelformat);
  printf("	vid_format->fmt.pix.sizeimage   =%d\n",	vid_format->fmt.pix.sizeimage );
  printf("	vid_format->fmt.pix.field       =%d\n",	vid_format->fmt.pix.field );
  printf("	vid_format->fmt.pix.bytesperline=%d\n",	vid_format->fmt.pix.bytesperline );
  printf("	vid_format->fmt.pix.colorspace  =%d\n",	vid_format->fmt.pix.colorspace );
}

class SendFrameArgs
{
public:
	unsigned imgLen;
	std::string pxFmt;
	unsigned width;
	unsigned height;

	SendFrameArgs()
	{
		imgLen = 0;
		width = 0;
		height = 0;
	}

	SendFrameArgs(const SendFrameArgs &in)
	{
		SendFrameArgs::operator=(in);
	}	

	const SendFrameArgs &operator=(const SendFrameArgs &in)
	{
		width = in.width;
		height = in.height;
		imgLen = in.imgLen;
		pxFmt = in.pxFmt;
		return *this;
	}
};

//*******************************************************************

Video_out::Video_out(const char *devNameIn) : Base_Video_Out()
{
	this->fdwr = 0;
	framesize = 0;
	stop = 0;
	stopped = 1;
	verbose = 1;
	this->devName = devNameIn;
	pthread_mutex_init(&lock, NULL);
	currentFrame = NULL;
	outputWidth = 640;
	outputHeight = 480;
	outputPxFmt = "YUYV";

	clock_gettime(CLOCK_MONOTONIC, &lastFrameTime);

	struct sigevent sevp;
	memset(&sevp, 0, sizeof(struct sigevent));
	sevp.sigev_notify = SIGEV_NONE;
	
}

Video_out::~Video_out()
{
	for(unsigned i=0; i<this->sendFrameBuffer.size(); i++)
	{
		delete [] this->sendFrameBuffer[i];
	}
	this->sendFrameBuffer.clear();

	if(this->currentFrame!=NULL)
		delete [] this->currentFrame;
	this->currentFrame = NULL;

	pthread_mutex_destroy(&lock);
}

void Video_out::SendFrameInternal()
{
	const char* buff = NULL;
	class SendFrameArgs args;
			
	pthread_mutex_lock(&this->lock);
	if(this->sendFrameBuffer.size()>=1)
	{
		//Get oldest frame
		buff = this->sendFrameBuffer[0];
		args = this->sendFrameArgs[0];

		//Remove frame from buffer
		this->sendFrameBuffer.erase(this->sendFrameBuffer.begin());
		this->sendFrameArgs.erase(this->sendFrameArgs.begin());
	}
	pthread_mutex_unlock(&this->lock);

	//Check time since previous frame send
	struct timespec tp;
	clock_gettime(CLOCK_MONOTONIC, &tp);
	long int secSinceLastFrame = tp.tv_sec - this->lastFrameTime.tv_sec;
	long int nsecSinceLastFrame = tp.tv_nsec - this->lastFrameTime.tv_nsec;
	if(nsecSinceLastFrame < 0)
	{
		secSinceLastFrame -= 1;
		nsecSinceLastFrame *= -1;
	}

	if(buff != NULL)
	{
		//Convert new frame to correct size and pixel format
		assert(strcmp(args.pxFmt.c_str(), "RGB24")==0);
		unsigned resizeBuffLen = this->outputWidth * this->outputHeight * 3;
		char *buffResize = new char[resizeBuffLen];
		memset(buffResize, 0, resizeBuffLen);
		for(unsigned x = 0; x < this->outputWidth; x++)
		{
			if (x >= args.width) continue;
			for(unsigned y = 0; y < this->outputHeight; y++)
			{
				if (y >= args.height) continue;
				buffResize[y * this->outputWidth * 3 + x * 3] = buff[y * args.width * 3 + x * 3];
				buffResize[y * this->outputWidth * 3 + x * 3 + 1] = buff[y * args.width * 3 + x * 3 + 1];
				buffResize[y * this->outputWidth * 3 + x * 3 + 2] = buff[y * args.width * 3 + x * 3 + 2];
			}
		}

		unsigned char *buffOut = NULL;
		unsigned buffOutLen = 0;
		DecodeFrame((unsigned char *)buffResize, resizeBuffLen, 
			args.pxFmt.c_str(),
			this->outputWidth, this->outputHeight,
			this->outputPxFmt.c_str(),
			&buffOut,
			&buffOutLen);

		assert(buffOutLen == this->framesize);

		//Replace current frame with new encoded frame
		if(this->currentFrame!=NULL)
			delete [] this->currentFrame;
		this->currentFrame = buffOut;

		delete [] buffResize;

	}

	//If we have no data, initialise with a blank frame
	if(this->currentFrame==NULL)
	{
		this->currentFrame = new unsigned char[this->framesize];
		memset(this->currentFrame, 0, this->framesize);
	}

	int timeElapsed = secSinceLastFrame>=1;

	if(timeElapsed || buff != NULL)
	{
		//Send frame update due to time elapse
		if(timeElapsed)
			printf("Write frame due to elapse time\n");
		write(this->fdwr, this->currentFrame, this->framesize);

		this->lastFrameTime = tp;
	}

	//Free image buffer
	if(buff!=NULL)
		delete [] buff;
}

void Video_out::Run()
{
	if(verbose) printf("Thread started: %s\n", this->devName.c_str());
	int running = 1;
	pthread_mutex_lock(&this->lock);
	this->stopped = 0;
	pthread_mutex_unlock(&this->lock);

	this->fdwr = open(this->devName.c_str(), O_RDWR);
	assert(fdwr >= 0);

	struct v4l2_capability vid_caps;
	int ret_code = ioctl(this->fdwr, VIDIOC_QUERYCAP, &vid_caps);
	assert(ret_code != -1);
	
	struct v4l2_format vid_format;
	memset(&vid_format, 0, sizeof(vid_format));

	ret_code = ioctl(this->fdwr, VIDIOC_G_FMT, &vid_format);
	if(verbose)print_format(&vid_format);

	int lw = 0;
	int fw = 0;
	if(strcmp(this->outputPxFmt.c_str(), "YVU420")==0)
	{
		lw = this->outputWidth; /* ??? */
		fw = ROUND_UP_4 (this->outputWidth) * ROUND_UP_2 (this->outputHeight);
		fw += 2 * ((ROUND_UP_8 (this->outputWidth) / 2) * (ROUND_UP_2 (this->outputHeight) / 2));
	}

	if(strcmp(this->outputPxFmt.c_str(), "YUYV")==0 
		|| strcmp(this->outputPxFmt.c_str(), "UYVY")==0 )
	{
		lw = (ROUND_UP_2 (this->outputWidth) * 2);
		fw = lw * this->outputHeight;
	}

	vid_format.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	vid_format.fmt.pix.width = this->outputWidth;
	vid_format.fmt.pix.height = this->outputHeight;
	vid_format.fmt.pix.pixelformat = 0;
	if(strcmp(this->outputPxFmt.c_str(), "YUYV")==0)
		vid_format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
	if(strcmp(this->outputPxFmt.c_str(), "UYVY")==0)
		vid_format.fmt.pix.pixelformat = V4L2_PIX_FMT_UYVY;
	if(strcmp(this->outputPxFmt.c_str(), "YVU420")==0)
		vid_format.fmt.pix.pixelformat = V4L2_PIX_FMT_YVU420;
	if(strcmp(this->outputPxFmt.c_str(), "RGB24")==0)
		vid_format.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB24;

	vid_format.fmt.pix.sizeimage = lw;
	vid_format.fmt.pix.field = V4L2_FIELD_NONE;
	vid_format.fmt.pix.bytesperline = fw;
	vid_format.fmt.pix.colorspace = V4L2_COLORSPACE_SRGB;

	if(verbose)print_format(&vid_format);

	ret_code = ioctl(this->fdwr, VIDIOC_S_FMT, &vid_format);

	assert(ret_code != -1);

	this->framesize = vid_format.fmt.pix.sizeimage;
	int linewidth = vid_format.fmt.pix.bytesperline;
	if(verbose)printf("frame: format=%s\tsize=%d\n", this->outputPxFmt.c_str(), framesize);

	try
	{
	while(running)
	{
		usleep(1000);

		this->SendFrameInternal();

		pthread_mutex_lock(&this->lock);
		try
		{
			running = !this->stop;
		}
		catch(std::exception &err)
		{
			if(verbose) printf("An exception has occured: %s\n", err.what());
			running = 0;
		}
		pthread_mutex_unlock(&this->lock);
	}
	}
	catch(std::exception &err)
	{
		if(verbose) printf("An exception has occured: %s\n", err.what());
	}

	if(verbose) printf("Thread stopping\n");
	pthread_mutex_lock(&this->lock);
	this->stopped = 1;
	pthread_mutex_unlock(&this->lock);
}

void Video_out::SendFrame(const char *imgIn, unsigned imgLen, const char *pxFmt, int width, int height)
{
	pthread_mutex_lock(&this->lock);
	if(verbose) printf("SendFrame %i %s %i %i\n", imgLen, pxFmt, width, height);

	//Take a shallow copy of the buffer and keep for worker thread
	char *buffCpy = new char[imgLen];
	memcpy(buffCpy, imgIn, imgLen);
	this->sendFrameBuffer.push_back(buffCpy);

	class SendFrameArgs sendFrameArgsTmp;
	sendFrameArgsTmp.imgLen = imgLen;
	sendFrameArgsTmp.pxFmt = pxFmt;
	sendFrameArgsTmp.width = width;
	sendFrameArgsTmp.height = height;
	this->sendFrameArgs.push_back(sendFrameArgsTmp);

	pthread_mutex_unlock(&this->lock);
}

void Video_out::Stop()
{
	pthread_mutex_lock(&this->lock);
	this->stop = 1;
	pthread_mutex_unlock(&this->lock);
}

int Video_out::WaitForStop()
{
	this->Stop();
	while(1)
	{
		pthread_mutex_lock(&this->lock);
		int s = this->stopped;
		pthread_mutex_unlock(&this->lock);

		if(s) return 1;
		usleep(10000);
	}
}

void Video_out::SetOutputSize(int width, int height)
{
	pthread_mutex_lock(&this->lock);
	this->outputWidth = width;
	this->outputHeight = height;
	pthread_mutex_unlock(&this->lock);
}

void Video_out::SetOutputPxFmt(const char *fmt)
{
	pthread_mutex_lock(&this->lock);
	this->outputPxFmt = fmt;
	pthread_mutex_unlock(&this->lock);
}

void *Video_out_manager_Worker_thread(void *arg)
{
	class Video_out *argobj = (class Video_out*) arg;
	argobj->Run();

	return NULL;
}

// *****************************************************************

std::vector<std::string> List_out_devices()
{
	std::vector<std::string> out;
	const char dir[] = "/dev";
	DIR *dp;
	struct dirent *dirp;
	if((dp  = opendir(dir)) == NULL) {
		printf("Error(%d) opening %s\n", errno, dir);
		return out;
	}

	while ((dirp = readdir(dp)) != NULL) {
		if (strncmp(dirp->d_name, "video", 5) != 0) continue;
		std::string tmp = "/dev/";
		tmp.append(dirp->d_name);
		out.push_back(tmp);
	}
	closedir(dp);
	return out;
}



