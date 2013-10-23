// python-v4l2capture
// Python extension to capture video with video4linux2
//
// 2009, 2010, 2011 Fredrik Portstrom, released into the public domain
// 2011, Joakim Gebart
// 2013, Tim Sheerman-Chase
// See README for license

#define USE_LIBV4L

#include <Python.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <sys/mman.h>
#include <string.h>
#include <string>
#include <sstream>
#include <map>
#include <vector>
#include <stdexcept>
#include <pthread.h>
#include <dirent.h>
#include "v4l2out.h"
#include "pixfmt.h"

#ifdef USE_LIBV4L
#include <libv4l2.h>
#else
#include <sys/ioctl.h>
#define v4l2_close close
#define v4l2_ioctl ioctl
#define v4l2_mmap mmap
#define v4l2_munmap munmap
#define v4l2_open open
#endif

#define ASSERT_OPEN if(self->fd < 0)					\
		{									\
			PyErr_SetString(PyExc_ValueError,					\
		"I/O operation on closed file");				\
			Py_RETURN_NONE;							\
		}

struct buffer {
	void *start;
	size_t length;
};

typedef struct {
	PyObject_HEAD
	int fd;
	struct buffer *buffers;
	int buffer_count;
} Video_device;

class Device_manager_cl{
public:
	PyObject_HEAD
	std::map<std::string, class Device_manager_Worker_thread_args *> *threadArgStore;
};
typedef Device_manager_cl Device_manager;

static PyObject *Device_manager_stop(Device_manager *self, PyObject *args);
static PyObject *Device_manager_close(Device_manager *self, PyObject *args);

struct capability {
	int id;
	const char *name;
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

static int my_ioctl(int fd, int request, void *arg, int utimeout = -1)
{
	// Retry ioctl until it returns without being interrupted.

	for(;;)
	{
		// Wait for frame until time out
		if(utimeout >= 0)
		{

			fd_set fds;
			FD_ZERO (&fds);
			FD_SET (fd, &fds);

			struct timeval tv;
				tv.tv_sec = 0;
				tv.tv_usec = utimeout;
			int r = select(fd+1, &fds, NULL, NULL, &tv);
			
			if(r == 0)
				return 1; //Timed out
		}

		//printf("call\n");
		int result = v4l2_ioctl(fd, request, arg);
		//printf("%d\n", result);

		if(!result)
		{
			//printf("ret\n");
			return 0;
		}

		if(errno == EAGAIN)
		{
			//printf("ret\n");
			return 1;
		}

		if(errno != EINTR)
		{
			return 1;
		}
		usleep(1000);
	}
}

static PyObject *InsertHuffmanTable(PyObject *self, PyObject *args)
{
	/* This converts an MJPEG frame into a standard JPEG binary
	MJPEG images omit the huffman table if the standard table
	is used. If it is missing, this function adds the table
	into the file structure. */

	if(PyTuple_Size(args) < 1)
	{
		PyErr_BadArgument();
		PyErr_Format(PyExc_TypeError, "Function requires 1 argument");
 		Py_RETURN_NONE;
	}

	PyObject *inBuffer = PyTuple_GetItem(args, 0);

	if(!PyString_Check(inBuffer))
	{
		PyErr_BadArgument();
		PyErr_Format(PyExc_TypeError, "Argument 1 must be a string.");
		//PyObject* type = PyObject_Type(inBuffer);
		//PyObject_Print(type, stdout, Py_PRINT_RAW);
		//Py_CLEAR(type);
		
 		Py_RETURN_NONE;
	}

	unsigned char* inBufferPtr = (unsigned char*)PyString_AsString(inBuffer);
	Py_ssize_t inBufferLen = PyString_Size(inBuffer);
	std::string outBuffer;

	InsertHuffmanTableCTypes((unsigned char*)inBufferPtr, inBufferLen, outBuffer);

	PyObject *outBufferPy = PyString_FromStringAndSize(outBuffer.c_str(), outBuffer.length());

	return outBufferPy;
}


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


class Device_manager_Worker_thread_args
{
public:
	Device_manager *self;
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

	Device_manager_Worker_thread_args(const char *devNameIn)
	{
		stop = 0;
		stopped = 1;
		deviceStarted = 0;
		this->devName = devNameIn;
		pthread_mutex_init(&lock, NULL);
		buffer_counts = 10;
		buffers = NULL;
		stopDeviceFlag = 0;
		closeDeviceFlag = 0;
		frameWidth = 0;
		frameHeight = 0;
		decodedFrameBuffMaxSize = 10;
		verbose = 1;
		targetFmt = "RGB24";
	}

	virtual ~Device_manager_Worker_thread_args()
	{
		if(deviceStarted)
		{
			this->StopDeviceInternal();
		}

		if(fd!=-1)
		{
			this->CloseDeviceInternal();
		}

		if(buffers) delete [] buffers;
		this->buffers = NULL;

		for(unsigned int i=0; i<decodedFrameBuff.size(); i++)
		{
			delete [] this->decodedFrameBuff[i];
		}
		this->decodedFrameBuff.clear();

		pthread_mutex_destroy(&lock);
	}

	void Stop()
	{
		pthread_mutex_lock(&this->lock);
		this->stop = 1;
		pthread_mutex_unlock(&this->lock);
	}

	void WaitForStop()
	{
		while(1)
		{
			pthread_mutex_lock(&this->lock);
			int s = this->stopped;
			pthread_mutex_unlock(&this->lock);

			if(s) return;
			usleep(10000);
		}
	}

	void OpenDevice()
	{
		pthread_mutex_lock(&this->lock);
		this->openDeviceFlag.push_back(this->devName.c_str());
		pthread_mutex_unlock(&this->lock);
	}

	void SetFormat(const char *fmt, int width, int height)
	{
		class SetFormatParams params;
		params.fmt = fmt;
		params.width = width;
		params.height = height;

		pthread_mutex_lock(&this->lock);
		this->setFormatFlags.push_back(params);
		pthread_mutex_unlock(&this->lock);
	}

	void StartDevice(int buffer_count)
	{
		pthread_mutex_lock(&this->lock);
		this->startDeviceFlag.push_back(buffer_count);
		pthread_mutex_unlock(&this->lock);
	}

	void StopDevice()
	{
		pthread_mutex_lock(&this->lock);
		this->stopDeviceFlag = 1;
		pthread_mutex_unlock(&this->lock);
	}

	void CloseDevice()
	{
		pthread_mutex_lock(&this->lock);
		this->closeDeviceFlag = 1;
		pthread_mutex_unlock(&this->lock);
	}

	int GetFrame(unsigned char **buffOut, class FrameMetaData *metaOut)
	{
		pthread_mutex_lock(&this->lock);
		if(this->decodedFrameBuff.size()==0)
		{
			//No frame found
			*buffOut = NULL;
			metaOut = NULL;
			pthread_mutex_unlock(&this->lock);
			return 0;
		}

		//Return frame
		*buffOut = this->decodedFrameBuff[0];
		*metaOut = this->decodedFrameMetaBuff[0];
		this->decodedFrameBuff.erase(this->decodedFrameBuff.begin());
		this->decodedFrameMetaBuff.erase(this->decodedFrameMetaBuff.begin());
		pthread_mutex_unlock(&this->lock);
		return 1;
	}

protected:
	int ReadFrame()
	{
		if(this->fd<0)
			throw std::runtime_error("File not open");

		if(this->buffers == NULL)
			throw std::runtime_error("Buffers have not been created");

		struct v4l2_buffer buffer;
		buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buffer.memory = V4L2_MEMORY_MMAP;

		if(my_ioctl(this->fd, VIDIOC_DQBUF, &buffer, 10000))
		{
			return 0;
		}

		unsigned char *rgbBuff = NULL;
		unsigned rgbBuffLen = 0;
		int ok = DecodeFrame((const unsigned char*)this->buffers[buffer.index].start, buffer.bytesused, 
			this->pxFmt.c_str(),
			this->frameWidth,
			this->frameHeight,
			this->targetFmt.c_str(), &rgbBuff, &rgbBuffLen);

		//Return a frame, decoded or not
		pthread_mutex_lock(&this->lock);
				
		class FrameMetaData meta;
		meta.width = this->frameWidth;
		meta.height = this->frameHeight;
		if(ok && rgbBuff != NULL)
		{
			meta.fmt = this->targetFmt;
			meta.buffLen = rgbBuffLen;
			this->decodedFrameBuff.push_back(rgbBuff);
		}
		else
		{
			//Make a copy of un-decodable buffer to return
			unsigned char* buffOut = new unsigned char[buffer.bytesused];
			memcpy(buffOut, this->buffers[buffer.index].start, buffer.bytesused);
			meta.fmt = this->pxFmt;
			meta.buffLen = buffer.bytesused;
			this->decodedFrameBuff.push_back(buffOut);
		}
		meta.sequence = buffer.sequence;
		meta.tv_sec = buffer.timestamp.tv_sec;
		meta.tv_usec = buffer.timestamp.tv_usec;

		this->decodedFrameMetaBuff.push_back(meta);
		while(this->decodedFrameBuff.size() > this->decodedFrameBuffMaxSize)
		{
			this->decodedFrameBuff.erase(this->decodedFrameBuff.begin());
			this->decodedFrameMetaBuff.erase(this->decodedFrameMetaBuff.begin());
		}
		pthread_mutex_unlock(&this->lock);

		//Queue buffer for next frame
		if(my_ioctl(this->fd, VIDIOC_QBUF, &buffer))
		{
			throw std::runtime_error("VIDIOC_QBUF failed");
		}

		return 1;
	}

	int OpenDeviceInternal()
	{
		if(verbose) printf("OpenDeviceInternal\n");
		//Open the video device.
		this->fd = v4l2_open(this->devName.c_str(), O_RDWR | O_NONBLOCK);

		if(fd < 0)
		{
			throw std::runtime_error("Error opening device");
		}

		this->deviceStarted = 0;
		if(verbose) printf("Done opening\n");
		return 1;
	}

	int SetFormatInternal(class SetFormatParams &args)
	{
		if(verbose) printf("SetFormatInternal\n");
		//int size_x, int size_y, const char *fmt;

		struct v4l2_format format;
		format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		format.fmt.pix.width = args.width;
		format.fmt.pix.height = args.height;
		format.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB24;

		if(strcmp(args.fmt.c_str(), "MJPEG")==0)
			format.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
		if(strcmp(args.fmt.c_str(), "RGB24")==0)
			format.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB24;
		if(strcmp(args.fmt.c_str(), "YUV420")==0)
			format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUV420;
		if(strcmp(args.fmt.c_str(), "YVU420")==0)
			format.fmt.pix.pixelformat = V4L2_PIX_FMT_YVU420;
		if(strcmp(args.fmt.c_str(), "YUYV")==0)
			format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;

		format.fmt.pix.field = V4L2_FIELD_NONE;
		format.fmt.pix.bytesperline = 0;

		if(my_ioctl(this->fd, VIDIOC_S_FMT, &format))
		{
			return 0;
		}

		//Store pixel format for decoding usage later
		//this->pxFmt = args.fmt;
		//this->frameWidth = args.width;
		//this->frameHeight = args.height;
		this->GetFormatInternal();

		return 1;
	}

	int GetFormatInternal()
	{
		struct v4l2_format format;
		format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		if(my_ioctl(this->fd, VIDIOC_G_FMT, &format))
		{
			return 0;
		}

		this->frameWidth = format.fmt.pix.width;
		this->frameHeight = format.fmt.pix.height;

		switch(format.fmt.pix.pixelformat)
		{
		case V4L2_PIX_FMT_MJPEG:
			this->pxFmt = "MJPEG";
			break;
		case V4L2_PIX_FMT_RGB24:
			this->pxFmt = "RGB24";
			break;
		case V4L2_PIX_FMT_YUV420:
			this->pxFmt = "YUV420";
			break;
		case V4L2_PIX_FMT_YVU420:
			this->pxFmt = "YVU420";
			break;
		case V4L2_PIX_FMT_YUYV:
			this->pxFmt = "YUYV";
			break;
		default:
			this->pxFmt = "Unknown ";
			std::ostringstream oss;
			oss << format.fmt.pix.pixelformat;
			this->pxFmt.append(oss.str());

			break;
		}

		if(verbose) printf("Current format %s %i %i\n", this->pxFmt.c_str(), this->frameWidth, this->frameHeight);
		return 1;
	}

	int StartDeviceInternal(int buffer_count = 10)
	{
		if(verbose) printf("StartDeviceInternal\n");
		//Check this device has not already been start
		if(this->fd==-1)
		{
			throw std::runtime_error("Device not open");
		}

		//Set other parameters for capture
		//TODO

		/*
		//Query current pixel format
		self.size_x, self.size_y, self.pixelFmt = self.video.get_format()

		//Set target frames per second
		self.fps = self.video.set_fps(reqFps)
		*/

		// Create a buffer to store image data in. This must be done before
		// calling 'start' if v4l2capture is compiled with libv4l2. Otherwise
		// raises IOError.

		if(this->pxFmt.length()==0)
		{
			//Get current pixel format
			//TODO
			int ret = GetFormatInternal();
			if(!ret) throw std::runtime_error("Could not determine image format");
		}

		struct v4l2_requestbuffers reqbuf;
		reqbuf.count = buffer_count;
		reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		reqbuf.memory = V4L2_MEMORY_MMAP;

		if(my_ioctl(this->fd, VIDIOC_REQBUFS, &reqbuf))
		{
			throw std::runtime_error("VIDIOC_REQBUFS failed");
		}

		if(!reqbuf.count)
		{
			throw std::runtime_error("Not enough buffer memory");
		}

		this->buffers = new struct buffer [reqbuf.count];

		if(this->buffers == NULL)
		{
			throw std::runtime_error("Failed to allocate buffer memory");
		}

		for(unsigned int i = 0; i < reqbuf.count; i++)
		{
			struct v4l2_buffer buffer;
			buffer.index = i;
			buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buffer.memory = V4L2_MEMORY_MMAP;

			if(my_ioctl(fd, VIDIOC_QUERYBUF, &buffer))
			{
				throw std::runtime_error("VIDIOC_QUERYBUF failed");
			}

			this->buffers[i].length = buffer.length;
			this->buffers[i].start = v4l2_mmap(NULL, buffer.length,
			PROT_READ | PROT_WRITE, MAP_SHARED, fd, buffer.m.offset);

			if(this->buffers[i].start == MAP_FAILED)
			{
				throw std::runtime_error("v4l2_mmap failed");
			}
		}

		this->buffer_counts = reqbuf.count;

		// Send the buffer to the device. Some devices require this to be done
		// before calling 'start'.

		for(int i = 0; i < buffer_count; i++)
		{
			struct v4l2_buffer buffer;
			buffer.index = i;
			buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buffer.memory = V4L2_MEMORY_MMAP;

			if(my_ioctl(fd, VIDIOC_QBUF, &buffer))
			{
				//This may fail with some devices but does not seem to be harmful.
			}
		}

		// Start the device. This lights the LED if it's a camera that has one.
		enum v4l2_buf_type type;
		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

		if(my_ioctl(fd, VIDIOC_STREAMON, &type))
		{
			throw std::runtime_error("VIDIOC_STREAMON failed");
		}

		this->deviceStarted = 1;
		if(verbose) printf("Started ok\n");
		return 1;
	}

	void StopDeviceInternal()
	{
		if(verbose) printf("StopDeviceInternal\n");
		if(this->fd==-1)
		{
			throw std::runtime_error("Device not started");
		}

		//Signal V4l2 api
		enum v4l2_buf_type type;
		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

		if(my_ioctl(this->fd, VIDIOC_STREAMOFF, &type))
		{
			throw std::runtime_error("VIDIOC_STREAMOFF failed");
		}

		this->deviceStarted = 0;
	}

	int CloseDeviceInternal()
	{
		if(verbose) printf("CloseDeviceInternal\n");
		if(this->fd == -1)
		{
			throw std::runtime_error("Device not open");
		}

		if(this->deviceStarted)
			StopDeviceInternal();

		if(this->buffers!= NULL)
		{
			for(int i = 0; i < this->buffer_counts; i++)
			{
				v4l2_munmap(this->buffers[i].start, this->buffers[i].length);	
			}
			delete [] this->buffers;
		}
		this->buffers = NULL;

		//Release memory
		v4l2_close(fd);
		fd = -1;
		return 1;
	}

public:
	void Run()
	{
		if(verbose) printf("Thread started: %s\n", this->devName.c_str());
		int running = 1;
		pthread_mutex_lock(&this->lock);
		this->stopped = 0;
		pthread_mutex_unlock(&this->lock);

		try
		{
		while(running)
		{
			//printf("Sleep\n");
			usleep(1000);

			if(deviceStarted) this->ReadFrame();

			pthread_mutex_lock(&this->lock);
			try
			{

			if(this->openDeviceFlag.size() > 0)
			{
				std::string devName = this->openDeviceFlag[this->openDeviceFlag.size()-1];
				this->openDeviceFlag.pop_back();
				this->OpenDeviceInternal();
			}

			if(this->setFormatFlags.size() > 0
				&& this->openDeviceFlag.size() == 0)
			{
				class SetFormatParams params = this->setFormatFlags[this->setFormatFlags.size()-1];
				this->setFormatFlags.pop_back();
				this->SetFormatInternal(params);
			}

			if(this->startDeviceFlag.size() > 0 
				&& this->openDeviceFlag.size() == 0
				&& this->setFormatFlags.size() == 0)
			{
				int buffer_count = this->startDeviceFlag[this->startDeviceFlag.size()-1];
				this->startDeviceFlag.pop_back();
				this->StartDeviceInternal(buffer_count);
			}

			if(this->stopDeviceFlag 
				&& this->openDeviceFlag.size() == 0
				&& this->setFormatFlags.size() == 0 
				&& this->startDeviceFlag.size() == 0)
			{
				this->StopDeviceInternal();
				this->stopDeviceFlag = 0;
			}

			if(this->closeDeviceFlag 
				&& this->openDeviceFlag.size() == 0 
				&& this->setFormatFlags.size() == 0
				&& this->startDeviceFlag.size() == 0
				&& !this->stopDeviceFlag)
			{
				this->CloseDeviceInternal();
				this->closeDeviceFlag = 0;
			}
		
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
	};
};

void *Device_manager_Worker_thread(void *arg)
{
	class Device_manager_Worker_thread_args *argobj = (class Device_manager_Worker_thread_args*) arg;
	argobj->Run();

	return NULL;
}

// **********************************************************************

static void Device_manager_dealloc(Device_manager *self)
{
	//Stop high level threads
	for(std::map<std::string, class Device_manager_Worker_thread_args *>::iterator it = self->threadArgStore->begin(); 
		it != self->threadArgStore->end(); it++)
	{
		PyObject *args = PyTuple_New(1);
		PyTuple_SetItem(args, 0, PyString_FromString(it->first.c_str()));
		Device_manager_stop(self, args);
		Py_DECREF(args);
	}

	delete self->threadArgStore;
	self->ob_type->tp_free((PyObject *)self);
}

static int Device_manager_init(Device_manager *self, PyObject *args,
		PyObject *kwargs)
{
	self->threadArgStore = new std::map<std::string, class Device_manager_Worker_thread_args*>;
	return 0;
}

static PyObject *Device_manager_open(Device_manager *self, PyObject *args)
{
	//Process arguments
	const char *devarg = "/dev/video0";
	if(PyTuple_Size(args) >= 1)
	{
		PyObject *pydevarg = PyTuple_GetItem(args, 0);
		devarg = PyString_AsString(pydevarg);
	}

	//Check this device has not already been opened
	std::map<std::string, class Device_manager_Worker_thread_args *>::iterator it = self->threadArgStore->find(devarg);
	if(it!=self->threadArgStore->end())
	{
		PyErr_Format(PyExc_RuntimeError, "Device already opened.");
 		Py_RETURN_NONE;
	}

	pthread_t thread;
	Device_manager_Worker_thread_args *threadArgs = new Device_manager_Worker_thread_args(devarg);
	(*self->threadArgStore)[devarg] = threadArgs;
	threadArgs->self = self;
	pthread_create(&thread, NULL, Device_manager_Worker_thread, threadArgs);

	threadArgs->OpenDevice();

	Py_RETURN_NONE;
}


static PyObject *Device_manager_set_format(Device_manager *self, PyObject *args)
{
	int size_x;
	int size_y;
	const char *fmt = NULL;
	const char *devarg = NULL;

	if(!PyArg_ParseTuple(args, "sii|s", &devarg, &size_x, &size_y, &fmt))
	{
		Py_RETURN_NONE;
	}

	class Device_manager_Worker_thread_args *threadArgs = (*self->threadArgStore)[devarg];
	threadArgs->SetFormat(fmt, size_x, size_y);

	Py_RETURN_NONE;
}

static PyObject *Device_manager_Start(Device_manager *self, PyObject *args)
{

	//Process arguments
	const char *devarg = "/dev/video0";
	if(PyTuple_Size(args) >= 1)
	{
		PyObject *pydevarg = PyTuple_GetItem(args, 0);
		devarg = PyString_AsString(pydevarg);
	}

	long buffer_count = 10;
	if(PyTuple_Size(args) >= 4)
	{
		PyObject *pybufferarg = PyTuple_GetItem(args, 4);
		buffer_count = PyInt_AsLong(pybufferarg);
	}

	class Device_manager_Worker_thread_args *threadArgs = (*self->threadArgStore)[devarg];
	threadArgs->StartDevice(buffer_count);
	
	Py_RETURN_NONE;
}

static PyObject *Device_manager_Get_frame(Device_manager *self, PyObject *args)
{

	//Process arguments
	const char *devarg = "/dev/video0";
	if(PyTuple_Size(args) >= 1)
	{
		PyObject *pydevarg = PyTuple_GetItem(args, 0);
		devarg = PyString_AsString(pydevarg);
	}

	class Device_manager_Worker_thread_args *threadArgs = (*self->threadArgStore)[devarg];
	unsigned char *buffOut = NULL; 
	class FrameMetaData metaOut;

	int ok = threadArgs->GetFrame(&buffOut, &metaOut);
	if(ok && buffOut != NULL)
	{
		//Format output to python
		PyObject *pymeta = PyDict_New();
		PyDict_SetItemString(pymeta, "width", PyInt_FromLong(metaOut.width));
		PyDict_SetItemString(pymeta, "height", PyInt_FromLong(metaOut.height));
		PyDict_SetItemString(pymeta, "format", PyString_FromString(metaOut.fmt.c_str()));
		PyDict_SetItemString(pymeta, "sequence", PyInt_FromLong(metaOut.sequence));
		PyDict_SetItemString(pymeta, "tv_sec", PyInt_FromLong(metaOut.tv_sec));
		PyDict_SetItemString(pymeta, "tv_usec", PyInt_FromLong(metaOut.tv_usec));

		PyObject *out = PyTuple_New(2);
		PyTuple_SetItem(out, 0, PyByteArray_FromStringAndSize((char *)buffOut, metaOut.buffLen));
		PyTuple_SetItem(out, 1, pymeta);

		delete [] buffOut;
		return out;
	}
	
	Py_RETURN_NONE;
}

static PyObject *Device_manager_stop(Device_manager *self, PyObject *args)
{
	//Process arguments
	const char *devarg = "/dev/video0";
	if(PyTuple_Size(args) >= 1)
	{
		PyObject *pydevarg = PyTuple_GetItem(args, 0);
		devarg = PyString_AsString(pydevarg);
	}

	class Device_manager_Worker_thread_args *threadArgs = (*self->threadArgStore)[devarg];
	threadArgs->StopDevice();

	Py_RETURN_NONE;
}

static PyObject *Device_manager_close(Device_manager *self, PyObject *args)
{
	//Process arguments
	const char *devarg = "/dev/video0";
	if(PyTuple_Size(args) >= 1)
	{
		PyObject *pydevarg = PyTuple_GetItem(args, 0);
		devarg = PyString_AsString(pydevarg);
	}

	class Device_manager_Worker_thread_args *threadArgs = (*self->threadArgStore)[devarg];
	threadArgs->CloseDevice();

	//Stop worker thread
	threadArgs->Stop();

	//Release memeory
	threadArgs->WaitForStop();
	delete threadArgs;
	self->threadArgStore->erase(devarg);

	Py_RETURN_NONE;
}

static PyObject *Device_manager_list_devices(Device_manager *self)
{
	PyObject *out = PyList_New(0);
	const char dir[] = "/dev";
	DIR *dp;
	struct dirent *dirp;
	if((dp  = opendir(dir)) == NULL) {
		printf("Error(%d) opening %s\n", errno, dir);
		Py_RETURN_NONE;
	}

	while ((dirp = readdir(dp)) != NULL) {
		if (strncmp(dirp->d_name, "video", 5) != 0) continue;
		std::string tmp = "/dev/";
		tmp.append(dirp->d_name);
		PyList_Append(out, PyString_FromString(tmp.c_str()));
	}
	closedir(dp);

	PyList_Sort(out);
	return out;
}

// *********************************************************************

static PyMethodDef Device_manager_methods[] = {
	{"open", (PyCFunction)Device_manager_open, METH_VARARGS,
			 "open(dev = '\\dev\\video0')\n\n"
			 "Open video capture."},
	{"set_format", (PyCFunction)Device_manager_set_format, METH_VARARGS,
			 "set_format(dev, size_x, size_y, pixel_format='RGB24') -> size_x, size_y\n\n"
			 "Request the video device to set image size and format. The device may "
			 "choose another size than requested and will return its choice. The "
			 "pixel format may be either RGB24, YUV420 or MJPEG."},
	{"start", (PyCFunction)Device_manager_Start, METH_VARARGS,
			 "start(dev = '\\dev\\video0', reqSize=(640, 480), reqFps = 30, fmt = 'MJPEG\', buffer_count = 10)\n\n"
			 "Start video capture."},
	{"get_frame", (PyCFunction)Device_manager_Get_frame, METH_VARARGS,
			 "start(dev = '\\dev\\video0'\n\n"
			 "Get video frame."},
	{"stop", (PyCFunction)Device_manager_stop, METH_VARARGS,
			 "stop(dev = '\\dev\\video0')\n\n"
			 "Stop video capture."},
	{"close", (PyCFunction)Device_manager_close, METH_VARARGS,
			 "close(dev = '\\dev\\video0')\n\n"
			 "Close video device. Subsequent calls to other methods will fail."},
	{"list_devices", (PyCFunction)Device_manager_list_devices, METH_NOARGS,
			 "list_devices()\n\n"
			 "List available capture devices."},
	{NULL}
};

static PyTypeObject Device_manager_type = {
	PyObject_HEAD_INIT(NULL)
			0, "v4l2capture.Device_manager", sizeof(Device_manager), 0,
			(destructor)Device_manager_dealloc, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, Py_TPFLAGS_DEFAULT, "Device_manager(path)\n\nOpens the video device at "
			"the given path and returns an object that can capture images. The "
			"constructor and all methods except close may raise IOError.", 0, 0, 0,
			0, 0, 0, Device_manager_methods, 0, 0, 0, 0, 0, 0, 0,
			(initproc)Device_manager_init
};

static PyMethodDef Video_out_manager_methods[] = {
	{"open", (PyCFunction)Video_out_manager_open, METH_VARARGS,
			 "open(dev = '\\dev\\video0')\n\n"
			 "Open video output."},
	{"send_frame", (PyCFunction)Video_out_manager_Send_frame, METH_VARARGS,
			 "send_frame(dev = '\\dev\\video0', img, pixel_format)\n\n"
			 "Send frame to video stream output."},
	{"close", (PyCFunction)Video_out_manager_close, METH_VARARGS,
			 "close(dev = '\\dev\\video0')\n\n"
			 "Close video device. Subsequent calls to other methods will fail."},
	{NULL}
};

static PyTypeObject Video_out_manager_type = {
	PyObject_HEAD_INIT(NULL)
			0, "v4l2capture.Video_out_manager", sizeof(Video_out_manager), 0,
			(destructor)Video_out_manager_dealloc, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, Py_TPFLAGS_DEFAULT, "Video_out_manager(path)\n\nOpens the video device at "
			"the given path and returns an object that can capture images. The "
			"constructor and all methods except close may raise IOError.", 0, 0, 0,
			0, 0, 0, Video_out_manager_methods, 0, 0, 0, 0, 0, 0, 0,
			(initproc)Video_out_manager_init
};

// *********************************************************************

static PyMethodDef module_methods[] = {
	{ "InsertHuffmanTable", (PyCFunction)InsertHuffmanTable, METH_VARARGS, NULL },
	{ NULL, NULL, 0, NULL }
};

PyMODINIT_FUNC initv4l2capture(void)
{
	Device_manager_type.tp_new = PyType_GenericNew;
	Video_out_manager_type.tp_new = PyType_GenericNew;

	if(PyType_Ready(&Device_manager_type) < 0)
		{
			return;
		}
	if(PyType_Ready(&Video_out_manager_type) < 0)
		{
			return;
		}

	PyObject *module = Py_InitModule3("v4l2capture", module_methods,
			"Capture video with video4linux2.");

	if(!module)
		{
			return;
		}

	Py_INCREF(&Device_manager_type);
	PyModule_AddObject(module, "Device_manager", (PyObject *)&Device_manager_type);
	PyModule_AddObject(module, "Video_out_manager", (PyObject *)&Video_out_manager_type);

}
