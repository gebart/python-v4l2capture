
#include "v4l2capture.h"

#define USE_LIBV4L

#include <fcntl.h>
#include <linux/videodev2.h>
#include <sys/mman.h>
#include <string.h>
#include <string>
#include <sstream>
#include <map>
#include <stdexcept>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>

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

int my_ioctl(int fd, int request, void *arg, int utimeout = -1)
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

// ***************************************************************************

Device_manager_Worker_thread_args::Device_manager_Worker_thread_args(const char *devNameIn)
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
	verbose = 0;
	targetFmt = "RGB24";
}

Device_manager_Worker_thread_args::~Device_manager_Worker_thread_args()
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

void Device_manager_Worker_thread_args::Stop()
{
	pthread_mutex_lock(&this->lock);
	this->stop = 1;
	pthread_mutex_unlock(&this->lock);
}

void Device_manager_Worker_thread_args::WaitForStop()
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

void Device_manager_Worker_thread_args::OpenDevice()
{
	pthread_mutex_lock(&this->lock);
	this->openDeviceFlag.push_back(this->devName.c_str());
	pthread_mutex_unlock(&this->lock);
}

void Device_manager_Worker_thread_args::SetFormat(const char *fmt, int width, int height)
{
	class SetFormatParams params;
	params.fmt = fmt;
	params.width = width;
	params.height = height;

	pthread_mutex_lock(&this->lock);
	this->setFormatFlags.push_back(params);
	pthread_mutex_unlock(&this->lock);
}

void Device_manager_Worker_thread_args::StartDevice(int buffer_count)
{
	pthread_mutex_lock(&this->lock);
	this->startDeviceFlag.push_back(buffer_count);
	pthread_mutex_unlock(&this->lock);
}

void Device_manager_Worker_thread_args::StopDevice()
{
	pthread_mutex_lock(&this->lock);
	this->stopDeviceFlag = 1;
	pthread_mutex_unlock(&this->lock);
}

void Device_manager_Worker_thread_args::CloseDevice()
{
	pthread_mutex_lock(&this->lock);
	this->closeDeviceFlag = 1;
	pthread_mutex_unlock(&this->lock);
}

int Device_manager_Worker_thread_args::GetFrame(unsigned char **buffOut, class FrameMetaData *metaOut)
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

int Device_manager_Worker_thread_args::ReadFrame()
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

int Device_manager_Worker_thread_args::OpenDeviceInternal()
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

int Device_manager_Worker_thread_args::SetFormatInternal(class SetFormatParams &args)
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

int Device_manager_Worker_thread_args::GetFormatInternal()
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

int Device_manager_Worker_thread_args::StartDeviceInternal(int buffer_count = 10)
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

void Device_manager_Worker_thread_args::StopDeviceInternal()
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

int Device_manager_Worker_thread_args::CloseDeviceInternal()
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

void Device_manager_Worker_thread_args::Run()
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
}

void *Device_manager_Worker_thread(void *arg)
{
	class Device_manager_Worker_thread_args *argobj = (class Device_manager_Worker_thread_args*) arg;
	argobj->Run();

	return NULL;
}

