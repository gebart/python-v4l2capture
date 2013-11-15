// libvideolive
// Python extension to capture and stream video 
//
// 2009, 2010, 2011 Fredrik Portstrom, released into the public domain
// 2011, Joakim Gebart
// 2013, Tim Sheerman-Chase
// See README for license

#include <Python.h>
#include <fcntl.h>
#include <string.h>
#include <string>
#include <sstream>
#include <map>
#include <vector>
#include <stdexcept>
#include <pthread.h>
#include <dirent.h>
#include "pixfmt.h"
#include "v4l2capture.h"
#include "videoout.h"

typedef struct {
	PyObject_HEAD
	int fd;
	struct buffer *buffers;
	int buffer_count;
} Video_device;

class Device_manager_cl{
public:
	PyObject_HEAD
	std::map<std::string, class Base_Video_In *> *threadArgStore;
};
typedef Device_manager_cl Device_manager;

static PyObject *Device_manager_stop(Device_manager *self, PyObject *args);
static PyObject *Device_manager_close(Device_manager *self, PyObject *args);

// *********************************************************************

PyObject *InsertHuffmanTable(PyObject *self, PyObject *args)
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

static void Device_manager_dealloc(Device_manager *self)
{
	//Stop high level threads
	for(std::map<std::string, class Base_Video_In *>::iterator it = self->threadArgStore->begin(); 
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
	self->threadArgStore = new std::map<std::string, class Base_Video_In*>;
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
	std::map<std::string, class Base_Video_In *>::iterator it = self->threadArgStore->find(devarg);
	if(it!=self->threadArgStore->end())
	{
		PyErr_Format(PyExc_RuntimeError, "Device already opened.");
 		Py_RETURN_NONE;
	}

	pthread_t thread;
	Video_in_Manager *threadArgs = new Video_in_Manager(devarg);
	(*self->threadArgStore)[devarg] = threadArgs;
	pthread_create(&thread, NULL, Video_in_Worker_thread, threadArgs);

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

	//Check this device is valid
	std::map<std::string, class Base_Video_In *>::iterator it = self->threadArgStore->find(devarg);
	if(it==self->threadArgStore->end())
	{
		PyErr_Format(PyExc_RuntimeError, "Device already not ready.");
 		Py_RETURN_NONE;
	}

	class Base_Video_In *threadArgs = (*self->threadArgStore)[devarg];
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

	//Check this device is valid
	std::map<std::string, class Base_Video_In *>::iterator it = self->threadArgStore->find(devarg);
	if(it==self->threadArgStore->end())
	{
		PyErr_Format(PyExc_RuntimeError, "Device already not ready.");
 		Py_RETURN_NONE;
	}

	class Base_Video_In *threadArgs = (*self->threadArgStore)[devarg];
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

	//Check this device is valid
	std::map<std::string, class Base_Video_In *>::iterator it = self->threadArgStore->find(devarg);
	if(it==self->threadArgStore->end())
	{
		PyErr_Format(PyExc_RuntimeError, "Device already not ready.");
 		Py_RETURN_NONE;
	}

	class Base_Video_In *threadArgs = (*self->threadArgStore)[devarg];
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

	//Check this device is valid
	std::map<std::string, class Base_Video_In *>::iterator it = self->threadArgStore->find(devarg);
	if(it==self->threadArgStore->end())
	{
		PyErr_Format(PyExc_RuntimeError, "Device already not ready.");
 		Py_RETURN_NONE;
	}

	class Base_Video_In *threadArgs = (*self->threadArgStore)[devarg];
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

	//Check this device is valid
	std::map<std::string, class Base_Video_In *>::iterator it = self->threadArgStore->find(devarg);
	if(it==self->threadArgStore->end())
	{
		PyErr_Format(PyExc_RuntimeError, "Device already not ready.");
 		Py_RETURN_NONE;
	}

	class Base_Video_In *threadArgs = (*self->threadArgStore)[devarg];
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
			 "open(dev = '\\dev\\video0', pixel_format, width, height)\n\n"
			 "Open video output."},
	{"send_frame", (PyCFunction)Video_out_manager_Send_frame, METH_VARARGS,
			 "send_frame(dev = '\\dev\\video0', img, pixel_format, width, height)\n\n"
			 "Send frame to video stream output."},
	{"close", (PyCFunction)Video_out_manager_close, METH_VARARGS,
			 "close(dev = '\\dev\\video0')\n\n"
			 "Close video device. Subsequent calls to other methods will fail."},
	{"list_devices", (PyCFunction)Video_out_manager_list_devices, METH_NOARGS,
			 "list_devices()\n\n"
			 "List available capture devices."},
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

PyMODINIT_FUNC initvideolive(void)
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

	PyObject *module = Py_InitModule3("videolive", module_methods,
			"Capture and stream video.");

	if(!module)
		{
			return;
		}

	Py_INCREF(&Device_manager_type);
	PyModule_AddObject(module, "Device_manager", (PyObject *)&Device_manager_type);
	PyModule_AddObject(module, "Video_out_manager", (PyObject *)&Video_out_manager_type);

}
