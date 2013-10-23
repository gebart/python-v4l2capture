
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include <vector>
#include "v4l2out.h"

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

//*******************************************************************

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

class Video_out
{
public:
	Video_out_manager *self;
	std::string devName;
	int stop;
	int stopped;
	pthread_mutex_t lock;
	int verbose;
	std::vector<class SendFrameArgs> sendFrameArgs;
	std::vector<const char *> sendFrameBuffer;

	Video_out(const char *devNameIn)
	{
		stop = 0;
		stopped = 1;
		verbose = 1;
		this->devName = devNameIn;
		pthread_mutex_init(&lock, NULL);

	}

	virtual ~Video_out()
	{
		for(unsigned i=0; i<this->sendFrameBuffer.size(); i++)
		{
			delete [] this->sendFrameBuffer[i];
		}
		this->sendFrameBuffer.clear();

		pthread_mutex_destroy(&lock);
	}

protected:

	void SendFrameInternal(class SendFrameArgs *args)
	{

	}

public:
	void Run()
	{
		if(verbose) printf("Thread started: %s\n", this->devName.c_str());
		int running = 1;
		pthread_mutex_lock(&this->lock);
		this->stopped = 0;
		pthread_mutex_unlock(&this->lock);

		int fdwr = open(this->devName.c_str(), O_RDWR);
		assert(fdwr >= 0);

		struct v4l2_capability vid_caps;
		int ret_code = ioctl(fdwr, VIDIOC_QUERYCAP, &vid_caps);
		assert(ret_code != -1);
	
		struct v4l2_format vid_format;
		memset(&vid_format, 0, sizeof(vid_format));

		printf("a %d\n", vid_format.fmt.pix.sizeimage);

		ret_code = ioctl(fdwr, VIDIOC_G_FMT, &vid_format);
		if(verbose)print_format(&vid_format);

		#define FRAME_WIDTH 640
		#define FRAME_HEIGHT 480
		#define FRAME_FORMAT V4L2_PIX_FMT_YVU420
		int lw = FRAME_WIDTH; /* ??? */
		int fw = ROUND_UP_4 (FRAME_WIDTH) * ROUND_UP_2 (FRAME_HEIGHT);
		fw += 2 * ((ROUND_UP_8 (FRAME_WIDTH) / 2) * (ROUND_UP_2 (FRAME_HEIGHT) / 2));

		vid_format.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
		vid_format.fmt.pix.width = FRAME_WIDTH;
		vid_format.fmt.pix.height = FRAME_HEIGHT;
		vid_format.fmt.pix.pixelformat = FRAME_FORMAT;
		vid_format.fmt.pix.sizeimage = lw;
		//printf("test %d\n", vid_format.fmt.pix.sizeimage);
		vid_format.fmt.pix.field = V4L2_FIELD_NONE;
		vid_format.fmt.pix.bytesperline = fw;
		//printf("test2 %d\n", vid_format.fmt.pix.bytesperline);
		vid_format.fmt.pix.colorspace = V4L2_COLORSPACE_SRGB;

		printf("b %d\n", vid_format.fmt.pix.sizeimage);

		if(verbose)print_format(&vid_format);

		printf("b2 %d\n", vid_format.fmt.pix.sizeimage);

		ret_code = ioctl(fdwr, VIDIOC_S_FMT, &vid_format);

		printf("c %d\n", vid_format.fmt.pix.sizeimage);

		assert(ret_code != -1);

		int framesize = vid_format.fmt.pix.sizeimage;
		int linewidth = vid_format.fmt.pix.bytesperline;
		if(verbose)printf("frame: format=%d\tsize=%d\n", FRAME_FORMAT, framesize);
		printf("d %d\n", vid_format.fmt.pix.sizeimage);
		print_format(&vid_format);

		printf("test %d\n", framesize);
		printf("e %d\n", vid_format.fmt.pix.sizeimage);

		printf("testa %d\n", framesize);
		printf("f %d\n", vid_format.fmt.pix.sizeimage);

		__u8* buffer=(__u8*)malloc(sizeof(__u8)*framesize);
		memset(buffer, 0, framesize);

		printf("testb %d\n", framesize);

		try
		{
		while(running)
		{
			usleep(1000000);

			pthread_mutex_lock(&this->lock);		
			printf("%i\n", this->sendFrameBuffer.size());
			const char* buff = this->sendFrameBuffer[this->sendFrameBuffer.size()-1];
			class SendFrameArgs args = this->sendFrameArgs[this->sendFrameArgs.size()-1];

			//If necessary, remove old frames (but leave one in the buffer)
			//TODO

			pthread_mutex_unlock(&this->lock);

			printf("Write frame\n");
			write(fdwr, buffer, framesize);

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

	void SendFrame(const char *imgIn, unsigned imgLen, const char *pxFmt, int width, int height)
	{
		pthread_mutex_lock(&this->lock);
		//printf("x %i %s %i %i\n", imgLen, pxFmt, width, height);

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

	void Stop()
	{
		pthread_mutex_lock(&this->lock);
		this->stop = 1;
		pthread_mutex_unlock(&this->lock);
	}

	int WaitForStop()
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
};

void *Video_out_manager_Worker_thread(void *arg)
{
	class Video_out *argobj = (class Video_out*) arg;
	argobj->Run();

	return NULL;
}

// *****************************************************************

int Video_out_manager_init(Video_out_manager *self, PyObject *args,
		PyObject *kwargs)
{
	self->threads = new std::map<std::string, class Video_out *>;
	return 0;
}

void Video_out_manager_dealloc(Video_out_manager *self)
{
	//Stop high level threads
	for(std::map<std::string, class Video_out *>::iterator it = self->threads->begin(); 
		it != self->threads->end(); it++)
	{
		it->second->Stop();
		it->second->WaitForStop();
	}

	delete self->threads;
	self->threads = NULL;
	self->ob_type->tp_free((PyObject *)self);
}

PyObject *Video_out_manager_open(Video_out_manager *self, PyObject *args)
{
	//Process arguments
	const char *devarg = "/dev/video0";
	if(PyTuple_Size(args) >= 1)
	{
		PyObject *pydevarg = PyTuple_GetItem(args, 0);
		devarg = PyString_AsString(pydevarg);
	}

	//Create worker thread
	pthread_t thread;
	Video_out *threadArgs = new Video_out(devarg);
	(*self->threads)[devarg] = threadArgs;
	threadArgs->self = self;
	pthread_create(&thread, NULL, Video_out_manager_Worker_thread, threadArgs);

	Py_RETURN_NONE;
}

PyObject *Video_out_manager_Send_frame(Video_out_manager *self, PyObject *args)
{
	printf("Video_out_manager_Send_frame\n");
	//dev = '\\dev\\video0', img, pixel_format, width, height

	//Process arguments
	const char *devarg = NULL;
	const char *imgIn = NULL;
	const char *pxFmtIn = NULL;
	int widthIn = 0;
	int heightIn = 0;

	if(!PyArg_ParseTuple(args, "sssii", &devarg, &imgIn, &pxFmtIn, &widthIn, &heightIn))
	{
		PyErr_Format(PyExc_RuntimeError, "Incorrect arguments to function.");
		Py_RETURN_NONE;
	}
	PyObject *pyimg = PyTuple_GetItem(args, 1);
	Py_ssize_t imgLen = PyObject_Length(pyimg);

	std::map<std::string, class Video_out *>::iterator it = self->threads->find(devarg);

	if(it != self->threads->end())
	{
		it->second->SendFrame(imgIn, imgLen, pxFmtIn, widthIn, heightIn);
	}

	Py_RETURN_NONE;
}

PyObject *Video_out_manager_close(Video_out_manager *self, PyObject *args)
{
	//Process arguments
	const char *devarg = "/dev/video0";
	if(PyTuple_Size(args) >= 1)
	{
		PyObject *pydevarg = PyTuple_GetItem(args, 0);
		devarg = PyString_AsString(pydevarg);
	}

	//Stop worker thread
	std::map<std::string, class Video_out *>::iterator it = self->threads->find(devarg);

	if(it != self->threads->end())
	{
		it->second->Stop();
	}

	Py_RETURN_NONE;
}

