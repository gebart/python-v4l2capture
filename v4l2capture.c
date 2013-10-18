// python-v4l2capture
// Python extension to capture video with video4linux2
//
// 2009, 2010, 2011 Fredrik Portstrom
//
// I, the copyright holder of this file, hereby release it into the
// public domain. This applies worldwide. In case this is not legally
// possible: I grant anyone the right to use this work for any
// purpose, without any conditions, unless such conditions are
// required by law.

#define USE_LIBV4L

#include <Python.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <sys/mman.h>
#include <string.h>
#include <stdio.h> //Only used for debugging

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
      return NULL;							\
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

struct capability {
  int id;
  const char *name;
};

static struct capability capabilities[] = {
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
};

static int my_ioctl(int fd, int request, void *arg)
{
  // Retry ioctl until it returns without being interrupted.

  for(;;)
    {
      int result = v4l2_ioctl(fd, request, arg);

      if(!result)
	{
	  return 0;
	}

      if(errno != EINTR)
	{
	  PyErr_SetFromErrno(PyExc_IOError);
	  return 1;
	}
    }
}

static void Video_device_unmap(Video_device *self)
{
  int i;

  for(i = 0; i < self->buffer_count; i++)
    {
      v4l2_munmap(self->buffers[i].start, self->buffers[i].length);
    }
}

static void Video_device_dealloc(Video_device *self)
{
  if(self->fd >= 0)
    {
      if(self->buffers)
	{
	  Video_device_unmap(self);
	}

      v4l2_close(self->fd);
    }

  self->ob_type->tp_free((PyObject *)self);
}

static int Video_device_init(Video_device *self, PyObject *args,
    PyObject *kwargs)
{
  const char *device_path;

  if(!PyArg_ParseTuple(args, "s", &device_path))
    {
      return -1;
    }

  int fd = v4l2_open(device_path, O_RDWR | O_NONBLOCK);

  if(fd < 0)
    {
      PyErr_SetFromErrnoWithFilename(PyExc_IOError, (char *)device_path);
      return -1;
    }

  self->fd = fd;
  self->buffers = NULL;
  return 0;
}

static PyObject *Video_device_close(Video_device *self)
{
  if(self->fd >= 0)
    {
      if(self->buffers)
	{
	  Video_device_unmap(self);
	}

      v4l2_close(self->fd);
      self->fd = -1;
    }

  Py_RETURN_NONE;
}

static PyObject *Video_device_fileno(Video_device *self)
{
  ASSERT_OPEN;
  return PyInt_FromLong(self->fd);
}

static PyObject *Video_device_get_info(Video_device *self)
{
  ASSERT_OPEN;
  struct v4l2_capability caps;

  if(my_ioctl(self->fd, VIDIOC_QUERYCAP, &caps))
    {
      return NULL;
    }

  PyObject *set = PySet_New(NULL);

  if(!set)
    {
      return NULL;
    }

  struct capability *capability = capabilities;

  while((void *)capability < (void *)capabilities + sizeof(capabilities))
    {
      if(caps.capabilities & capability->id)
	{
	  PyObject *s = PyString_FromString(capability->name);

	  if(!s)
	    {
              Py_DECREF(set);
	      return NULL;
	    }

	  PySet_Add(set, s);
	}

      capability++;
    }

  return Py_BuildValue("sssO", caps.driver, caps.card, caps.bus_info, set);
}

static PyObject *Video_device_set_format(Video_device *self, PyObject *args)
{
  int size_x;
  int size_y;
  const char *fmt = NULL;

  if(!PyArg_ParseTuple(args, "ii|s", &size_x, &size_y, &fmt))
    {
      return NULL;
    }

  struct v4l2_format format;
  format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  format.fmt.pix.width = size_x;
  format.fmt.pix.height = size_y;
#ifdef USE_LIBV4L
  format.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB24;
  if(fmt != NULL && strcmp(fmt, "MJPEG")==0)
    format.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
  if(fmt != NULL && strcmp(fmt, "RGB24")==0)
    format.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB24;
  if(fmt != NULL && strcmp(fmt, "YUV420")==0)
    format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUV420;
#else
  format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
#endif
  format.fmt.pix.field = V4L2_FIELD_NONE;
  format.fmt.pix.bytesperline = 0;

  if(my_ioctl(self->fd, VIDIOC_S_FMT, &format))
    {
      return NULL;
    }

  return Py_BuildValue("ii", format.fmt.pix.width, format.fmt.pix.height);
}

static PyObject *Video_device_set_fps(Video_device *self, PyObject *args)
{
  int fps;
  if(!PyArg_ParseTuple(args, "i", &fps))
    {
      return NULL;
    }
  struct v4l2_streamparm setfps;
  memset(&setfps, 0, sizeof(struct v4l2_streamparm));
  setfps.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  setfps.parm.capture.timeperframe.numerator = 1;
  setfps.parm.capture.timeperframe.denominator = fps;
  if(my_ioctl(self->fd, VIDIOC_S_PARM, &setfps)){
  	return NULL;
  }
  return Py_BuildValue("i",setfps.parm.capture.timeperframe.denominator);
}

static PyObject *Video_device_get_format(Video_device *self)
{

	struct v4l2_format format;
	format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if(my_ioctl(self->fd, VIDIOC_G_FMT, &format))
	{
		return NULL;
	}

	PyObject *out = PyTuple_New(3);
	PyTuple_SetItem(out, 0, PyInt_FromLong(format.fmt.pix.width));
	PyTuple_SetItem(out, 1, PyInt_FromLong(format.fmt.pix.height));

	PyObject *pixFormatStr = NULL;
	switch(format.fmt.pix.pixelformat)
	{
	case V4L2_PIX_FMT_MJPEG:
		pixFormatStr = PyString_FromString("MJPEG");
		break;
	case V4L2_PIX_FMT_RGB24:
		pixFormatStr = PyString_FromString("RGB24");
		break;
	case V4L2_PIX_FMT_YUV420:
		pixFormatStr = PyString_FromString("YUV420");
		break;
	case V4L2_PIX_FMT_YUYV:
		pixFormatStr = PyString_FromString("YUYV");
		break;
	default:
		pixFormatStr = PyString_FromString("Unknown");
		break;
	}
	PyTuple_SetItem(out, 2, pixFormatStr);
	return out;

}

static PyObject *Video_device_start(Video_device *self)
{
  ASSERT_OPEN;
  enum v4l2_buf_type type;
  type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

  if(my_ioctl(self->fd, VIDIOC_STREAMON, &type))
    {
      return NULL;
    }

  Py_RETURN_NONE;
}

static PyObject *Video_device_stop(Video_device *self)
{
  ASSERT_OPEN;
  enum v4l2_buf_type type;
  type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

  if(my_ioctl(self->fd, VIDIOC_STREAMOFF, &type))
    {
      return NULL;
    }

  Py_RETURN_NONE;
}

static PyObject *Video_device_create_buffers(Video_device *self, PyObject *args)
{
  int buffer_count;

  if(!PyArg_ParseTuple(args, "I", &buffer_count))
    {
      return NULL;
    }

  ASSERT_OPEN;

  if(self->buffers)
    {
      PyErr_SetString(PyExc_ValueError, "Buffers are already created");
      return NULL;
    }

  struct v4l2_requestbuffers reqbuf;
  reqbuf.count = buffer_count;
  reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  reqbuf.memory = V4L2_MEMORY_MMAP;

  if(my_ioctl(self->fd, VIDIOC_REQBUFS, &reqbuf))
    {
      return NULL;
    }

  if(!reqbuf.count)
    {
      PyErr_SetString(PyExc_IOError, "Not enough buffer memory");
      return NULL;
    }

  self->buffers = malloc(reqbuf.count * sizeof(struct buffer));

  if(!self->buffers)
    {
      PyErr_NoMemory();
      return NULL;
    }

  int i;

  for(i = 0; i < reqbuf.count; i++)
    {
      struct v4l2_buffer buffer;
      buffer.index = i;
      buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      buffer.memory = V4L2_MEMORY_MMAP;

      if(my_ioctl(self->fd, VIDIOC_QUERYBUF, &buffer))
	{
	  return NULL;
	}

      self->buffers[i].length = buffer.length;
      self->buffers[i].start = v4l2_mmap(NULL, buffer.length,
	  PROT_READ | PROT_WRITE, MAP_SHARED, self->fd, buffer.m.offset);

      if(self->buffers[i].start == MAP_FAILED)
	{
	  PyErr_SetFromErrno(PyExc_IOError);
	  return NULL;
	}
    }

  self->buffer_count = i;
  Py_RETURN_NONE;
}

static PyObject *Video_device_queue_all_buffers(Video_device *self)
{
  if(!self->buffers)
    {
      ASSERT_OPEN;
      PyErr_SetString(PyExc_ValueError, "Buffers have not been created");
      return NULL;
    }

  int i;
  int buffer_count = self->buffer_count;

  for(i = 0; i < buffer_count; i++)
    {
      struct v4l2_buffer buffer;
      buffer.index = i;
      buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      buffer.memory = V4L2_MEMORY_MMAP;

      if(my_ioctl(self->fd, VIDIOC_QBUF, &buffer))
	{
	  return NULL;
	}
    }

  Py_RETURN_NONE;
}

static PyObject *Video_device_read_internal(Video_device *self, int queue, int return_timestamp)
{
  if(!self->buffers)
    {
      ASSERT_OPEN;
      PyErr_SetString(PyExc_ValueError, "Buffers have not been created");
      return NULL;
    }

  struct v4l2_buffer buffer;
  buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  buffer.memory = V4L2_MEMORY_MMAP;

  if(my_ioctl(self->fd, VIDIOC_DQBUF, &buffer))
    {
      return NULL;
    }

#ifdef USE_LIBV4L
  PyObject *result = PyString_FromStringAndSize(
      self->buffers[buffer.index].start, buffer.bytesused);

  if(!result)
    {
      return NULL;
    }
#else
  // Convert buffer from YUYV to RGB.
  // For the byte order, see: http://v4l2spec.bytesex.org/spec/r4339.htm
  // For the color conversion, see: http://v4l2spec.bytesex.org/spec/x2123.htm
  int length = buffer.bytesused * 6 / 4;
  PyObject *result = PyString_FromStringAndSize(NULL, length);

  if(!result)
    {
      return NULL;
    }

  char *rgb = PyString_AS_STRING(result);
  char *rgb_max = rgb + length;
  unsigned char *yuyv = self->buffers[buffer.index].start;

#define CLAMP(c) ((c) <= 0 ? 0 : (c) >= 65025 ? 255 : (c) >> 8)
  while(rgb < rgb_max)
    {
      int u = yuyv[1] - 128;
      int v = yuyv[3] - 128;
      int uv = 100 * u + 208 * v;
      u *= 516;
      v *= 409;

      int y = 298 * (yuyv[0] - 16);
      rgb[0] = CLAMP(y + v);
      rgb[1] = CLAMP(y - uv);
      rgb[2] = CLAMP(y + u);

      y = 298 * (yuyv[2] - 16);
      rgb[3] = CLAMP(y + v);
      rgb[4] = CLAMP(y - uv);
      rgb[5] = CLAMP(y + u);

      rgb += 6;
      yuyv += 4;
    }
#undef CLAMP
#endif

  PyObject *out = result;

  if(return_timestamp)
  {
    out = PyTuple_New(4);
    PyTuple_SetItem(out, 0, result);
    PyTuple_SetItem(out, 1, PyInt_FromLong(buffer.timestamp.tv_sec));
    PyTuple_SetItem(out, 2, PyInt_FromLong(buffer.timestamp.tv_usec));
    PyTuple_SetItem(out, 3, PyInt_FromLong(buffer.sequence));
  }

  if(queue && my_ioctl(self->fd, VIDIOC_QBUF, &buffer))
    {
      return NULL;
    }

  return out;
}

static PyObject *Video_device_read(Video_device *self, PyObject *args)
{
  int return_timestamp=0;

  if(!PyArg_ParseTuple(args, "|i", &return_timestamp))
    {
      return NULL;
    }

  return Video_device_read_internal(self, 0, return_timestamp);
}

static PyObject *Video_device_read_and_queue(Video_device *self, PyObject *args)
{
  int return_timestamp=0;

  if(!PyArg_ParseTuple(args, "|i", &return_timestamp))
    {
      return NULL;
    }

  return Video_device_read_internal(self, 1, return_timestamp);
}

// *********************************************************************

#define HUFFMAN_SEGMENT_LEN 420

const char huffmanSegment[HUFFMAN_SEGMENT_LEN+1] =
	"\xFF\xC4\x01\xA2\x00\x00\x01\x05\x01\x01\x01\x01"
	"\x01\x01\x00\x00\x00\x00\x00\x00\x00\x00\x01\x02"
	"\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x01\x00\x03"
	"\x01\x01\x01\x01\x01\x01\x01\x01\x01\x00\x00\x00"
	"\x00\x00\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09"
	"\x0A\x0B\x10\x00\x02\x01\x03\x03\x02\x04\x03\x05"
	"\x05\x04\x04\x00\x00\x01\x7D\x01\x02\x03\x00\x04"
	"\x11\x05\x12\x21\x31\x41\x06\x13\x51\x61\x07\x22"
	"\x71\x14\x32\x81\x91\xA1\x08\x23\x42\xB1\xC1\x15"
	"\x52\xD1\xF0\x24\x33\x62\x72\x82\x09\x0A\x16\x17"
	"\x18\x19\x1A\x25\x26\x27\x28\x29\x2A\x34\x35\x36"
	"\x37\x38\x39\x3A\x43\x44\x45\x46\x47\x48\x49\x4A"
	"\x53\x54\x55\x56\x57\x58\x59\x5A\x63\x64\x65\x66"
	"\x67\x68\x69\x6A\x73\x74\x75\x76\x77\x78\x79\x7A"
	"\x83\x84\x85\x86\x87\x88\x89\x8A\x92\x93\x94\x95"
	"\x96\x97\x98\x99\x9A\xA2\xA3\xA4\xA5\xA6\xA7\xA8"
	"\xA9\xAA\xB2\xB3\xB4\xB5\xB6\xB7\xB8\xB9\xBA\xC2"
	"\xC3\xC4\xC5\xC6\xC7\xC8\xC9\xCA\xD2\xD3\xD4\xD5"
	"\xD6\xD7\xD8\xD9\xDA\xE1\xE2\xE3\xE4\xE5\xE6\xE7"
	"\xE8\xE9\xEA\xF1\xF2\xF3\xF4\xF5\xF6\xF7\xF8\xF9"
	"\xFA\x11\x00\x02\x01\x02\x04\x04\x03\x04\x07\x05"
	"\x04\x04\x00\x01\x02\x77\x00\x01\x02\x03\x11\x04"
	"\x05\x21\x31\x06\x12\x41\x51\x07\x61\x71\x13\x22"
	"\x32\x81\x08\x14\x42\x91\xA1\xB1\xC1\x09\x23\x33"
	"\x52\xF0\x15\x62\x72\xD1\x0A\x16\x24\x34\xE1\x25"
	"\xF1\x17\x18\x19\x1A\x26\x27\x28\x29\x2A\x35\x36"
	"\x37\x38\x39\x3A\x43\x44\x45\x46\x47\x48\x49\x4A"
	"\x53\x54\x55\x56\x57\x58\x59\x5A\x63\x64\x65\x66"
	"\x67\x68\x69\x6A\x73\x74\x75\x76\x77\x78\x79\x7A"
	"\x82\x83\x84\x85\x86\x87\x88\x89\x8A\x92\x93\x94"
	"\x95\x96\x97\x98\x99\x9A\xA2\xA3\xA4\xA5\xA6\xA7"
	"\xA8\xA9\xAA\xB2\xB3\xB4\xB5\xB6\xB7\xB8\xB9\xBA"
	"\xC2\xC3\xC4\xC5\xC6\xC7\xC8\xC9\xCA\xD2\xD3\xD4"
	"\xD5\xD6\xD7\xD8\xD9\xDA\xE2\xE3\xE4\xE5\xE6\xE7"
	"\xE8\xE9\xEA\xF2\xF3\xF4\xF5\xF6\xF7\xF8\xF9\xFA";

int ReadJpegFrame(const unsigned char *data, unsigned offset, const unsigned char **twoBytesOut, unsigned *frameStartPosOut, unsigned *cursorOut)
{
	//Based on http://www.gdcl.co.uk/2013/05/02/Motion-JPEG.html
	//and https://en.wikipedia.org/wiki/JPEG

	*twoBytesOut = NULL;
	*frameStartPosOut = 0;
	*cursorOut = 0;
	unsigned cursor = offset;
	//Check frame start
	unsigned frameStartPos = offset;
	const unsigned char *twoBytes = &data[cursor];

	if (twoBytes[0] != 0xff)
	{
		//print "Error: found header", map(hex,twoBytes),"at position",cursor
		return 0;
	}

	cursor = 2 + cursor;

	//Handle padding
	int paddingByte = (twoBytes[0] == 0xff && twoBytes[1] == 0xff);
	if(paddingByte)
	{
		*twoBytesOut = twoBytes;
		*frameStartPosOut = frameStartPos;
		*cursorOut = cursor;
		return 1;
	}

	//Structure markers with 2 byte length
	int markHeader = (twoBytes[0] == 0xff && twoBytes[1] >= 0xd0 && twoBytes[1] <= 0xd9);
	if (markHeader)
	{
		*twoBytesOut = twoBytes;
		*frameStartPosOut = frameStartPos;
		*cursorOut = cursor;
		return 1;
	}

	//Determine length of compressed (entropy) data
	int compressedDataStart = (twoBytes[0] == 0xff && twoBytes[1] == 0xda);
	if (compressedDataStart)
	{
		unsigned sosLength = ((data[cursor] << 8) + data[cursor+1]);
		cursor += sosLength;

		//Seek through frame
		int run = 1;
		while(run)
		{
			unsigned char byte = data[cursor];
			cursor += 1;
			
			if(byte == 0xff)
			{
				unsigned char byte2 = data[cursor];
				cursor += 1;
				if(byte2 != 0x00)
				{
					if(byte2 >= 0xd0 && byte2 <= 0xd8)
					{
						//Found restart structure
						//print hex(byte), hex(byte2)
					}
					else
					{
						//End of frame
						run = 0;
						cursor -= 2;
					}
				}
				else
				{
					//Add escaped 0xff value in entropy data
				}	
			}
			else
			{
				
			}
		}

		*twoBytesOut = twoBytes;
		*frameStartPosOut = frameStartPos;
		*cursorOut = cursor;
		return 1;
	}

	//More cursor for all other segment types
	unsigned segLength = (data[cursor] << 8) + data[cursor+1];
	cursor += segLength;
	*twoBytesOut = twoBytes;
	*frameStartPosOut = frameStartPos;
	*cursorOut = cursor;
	return 1;
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
 		return NULL;
	}

	PyObject *inBuffer = PyTuple_GetItem(args, 0);

	if(!PyString_Check(inBuffer))
	{
		PyErr_BadArgument();
		PyErr_Format(PyExc_TypeError, "Argument 1 must be a string.");
		//PyObject* type = PyObject_Type(inBuffer);
		//PyObject_Print(type, stdout, Py_PRINT_RAW);
		//Py_CLEAR(type);
		
 		return NULL;
	}

	int parsing = 1;
	int frameStartPos = 0;
	int huffFound = 0;
	unsigned char* inBufferPtr = PyString_AsString(inBuffer);
	Py_ssize_t inBufferLen = PyString_Size(inBuffer);

	PyObject *outBuffer = PyString_FromString("");
	_PyString_Resize(&outBuffer, inBufferLen + HUFFMAN_SEGMENT_LEN);

	while(parsing)
	{
		//Check if we should stop
		if (frameStartPos >= inBufferLen)
		{
			parsing = 0;
			continue;
		}

		//Read the next segment
		const unsigned char *twoBytes = NULL;
		unsigned frameStartPos=0, frameEndPos=0;
		int ok = ReadJpegFrame(inBufferPtr, frameStartPos, &twoBytes, &frameStartPos, &frameEndPos);
		//if(verbose)
		//	print map(hex, twoBytes), frameStartPos, frameEndPos;

		//Stop if there is a serious error
		if(!ok)
		{
			parsing = 0;
			continue;
		}
	
		//Check if this segment is the compressed data
		if(twoBytes[0] == 0xff && twoBytes[1] == 0xda && !huffFound)
		{
			PyObject *substr = PyString_FromStringAndSize(huffmanSegment, HUFFMAN_SEGMENT_LEN);
			PyFile_WriteObject(substr, outBuffer, Py_PRINT_RAW);
			Py_CLEAR(substr);
		}

		//Check the type of frame
		if(twoBytes[0] == 0xff && twoBytes[1] == 0xc4)
			huffFound = 1;

		//Write current structure to output
		PyObject *substr = PyString_FromStringAndSize(&inBufferPtr[frameStartPos], frameEndPos - frameStartPos);
		PyFile_WriteObject(substr, outBuffer, Py_PRINT_RAW);
		Py_CLEAR(substr);

		//Move cursor
		frameStartPos = frameEndPos;
	}
	
	return outBuffer;
}

// *********************************************************************

static PyMethodDef Video_device_methods[] = {
  {"close", (PyCFunction)Video_device_close, METH_NOARGS,
       "close()\n\n"
       "Close video device. Subsequent calls to other methods will fail."},
  {"fileno", (PyCFunction)Video_device_fileno, METH_NOARGS,
       "fileno() -> integer \"file descriptor\".\n\n"
       "This enables video devices to be passed select.select for waiting "
       "until a frame is available for reading."},
  {"get_info", (PyCFunction)Video_device_get_info, METH_NOARGS,
       "get_info() -> driver, card, bus_info, capabilities\n\n"
       "Returns three strings with information about the video device, and one "
       "set containing strings identifying the capabilities of the video "
       "device."},
  {"set_format", (PyCFunction)Video_device_set_format, METH_VARARGS,
       "set_format(size_x, size_y, pixel_format='RGB24') -> size_x, size_y\n\n"
       "Request the video device to set image size and format. The device may "
       "choose another size than requested and will return its choice. The "
       "pixel format may be either RGB24, YUV420 or MJPEG."},
  {"get_format", (PyCFunction)Video_device_get_format, METH_NOARGS,
       "get_format() -> size_x, size_y\n\n"},
  {"set_fps", (PyCFunction)Video_device_set_fps, METH_VARARGS,
       "set_fps(fps) -> fps \n\n"
       "Request the video device to set frame per seconds.The device may "
       "choose another frame rate than requested and will return its choice. " },
  {"start", (PyCFunction)Video_device_start, METH_NOARGS,
       "start()\n\n"
       "Start video capture."},
  {"stop", (PyCFunction)Video_device_stop, METH_NOARGS,
       "stop()\n\n"
       "Stop video capture."},
  {"create_buffers", (PyCFunction)Video_device_create_buffers, METH_VARARGS,
       "create_buffers(count)\n\n"
       "Create buffers used for capturing image data. Can only be called once "
       "for each video device object."},
  {"queue_all_buffers", (PyCFunction)Video_device_queue_all_buffers,
       METH_NOARGS,
       "queue_all_buffers()\n\n"
       "Let the video device fill all buffers created."},
  {"read", (PyCFunction)Video_device_read, METH_VARARGS,
       "read(get_timestamp) -> string or tuple\n\n"
       "Reads image data from a buffer that has been filled by the video "
       "device. The image data is in RGB24, YUV420 or MJPEG format as decided by "
       "'set_format'. The buffer is removed from the queue. Fails if no buffer "
       "is filled. Use select.select to check for filled buffers. If "
       "get_timestamp is true, a tuple is turned containing (sec, microsec, "
       "sequence number)"},
  {"read_and_queue", (PyCFunction)Video_device_read_and_queue, METH_VARARGS,
       "read_and_queue(get_timestamp)\n\n"
       "Same as 'read', but adds the buffer back to the queue so the video "
       "device can fill it again."},
  {NULL}
};

static PyTypeObject Video_device_type = {
  PyObject_HEAD_INIT(NULL)
      0, "v4l2capture.Video_device", sizeof(Video_device), 0,
      (destructor)Video_device_dealloc, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, Py_TPFLAGS_DEFAULT, "Video_device(path)\n\nOpens the video device at "
      "the given path and returns an object that can capture images. The "
      "constructor and all methods except close may raise IOError.", 0, 0, 0,
      0, 0, 0, Video_device_methods, 0, 0, 0, 0, 0, 0, 0,
      (initproc)Video_device_init
};

static PyMethodDef module_methods[] = {
	{ "InsertHuffmanTable", (PyCFunction)InsertHuffmanTable, METH_VARARGS, NULL },
	{ NULL, NULL, 0, NULL }
};

PyMODINIT_FUNC initv4l2capture(void)
{
  Video_device_type.tp_new = PyType_GenericNew;

  if(PyType_Ready(&Video_device_type) < 0)
    {
      return;
    }

  PyObject *module = Py_InitModule3("v4l2capture", module_methods,
      "Capture video with video4linux2.");

  if(!module)
    {
      return;
    }

  Py_INCREF(&Video_device_type);
  PyModule_AddObject(module, "Video_device", (PyObject *)&Video_device_type);
}
