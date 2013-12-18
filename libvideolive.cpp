// libvideolive
// Python extension to capture and stream video 
//
// 2009, 2010, 2011 Fredrik Portstrom, released into the public domain
// 2011, Joakim Gebart
// 2013, Tim Sheerman-Chase
// See README for license

#include <Python.h>
#include <string.h>
#include <string>
#include <sstream>
#include <map>
#include <vector>
#include <stdexcept>
#include <pthread.h>
#include "pixfmt.h"
#include "videoout.h"
#include "videoin.h"
#include "videooutfile.h"

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
			0, "v4l2capture.Video_out_stream_manager", sizeof(Video_out_manager), 0,
			(destructor)Video_out_manager_dealloc, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, Py_TPFLAGS_DEFAULT, "Video_out_manager(path)\n\nOpens the video device at "
			"the given path and returns an object that can capture images. The "
			"constructor and all methods except close may raise IOError.", 0, 0, 0,
			0, 0, 0, Video_out_manager_methods, 0, 0, 0, 0, 0, 0, 0,
			(initproc)Video_out_manager_init
};

static PyMethodDef Video_out_file_manager_methods[] = {
	{"open", (PyCFunction)Video_out_file_manager_open, METH_VARARGS,
			 "open(filename = 'out.wmv', width, height)\n\n"
			 "Open video output."},
	{"send_frame", (PyCFunction)Video_out_file_manager_Send_frame, METH_VARARGS,
			 "send_frame(filename = 'out.wmv', img, pixel_format, width, height)\n\n"
			 "Send frame to video stream output."},
	{"close", (PyCFunction)Video_out_file_manager_close, METH_VARARGS,
			 "close(filename = 'out.wmv')\n\n"
			 "Close video device. Subsequent calls to other methods will fail."},
	{"set_frame_rate", (PyCFunction)Video_out_file_manager_Set_Frame_Rate, METH_VARARGS,
			 "set_frame_rate(filename = 'out.wmv', frame_rate)\n\n"
			 "Set output frame rate."},
	{"set_video_codec", (PyCFunction)Video_out_file_manager_Set_Video_Codec, METH_VARARGS,
			 "set_video_codec(filename = 'out.wmv', codec = 'H264', bitrate)\n\n"
			 "Set output video codec."},
	{"enable_real_time_frame_rate", (PyCFunction)Video_out_file_manager_Enable_Real_Time_Frame_Rate, METH_VARARGS,
			 "enable_real_time_frame_rate(filename = 'out.wmv', enable)\n\n"
			 "Set real time frame encoding."},
	{NULL}
};

static PyTypeObject Video_out_file_manager_type = {
	PyObject_HEAD_INIT(NULL)
			0, "v4l2capture.Video_out_file_manager", sizeof(Video_out_manager), 0,
			(destructor)Video_out_file_manager_dealloc, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, Py_TPFLAGS_DEFAULT, "Video_out_manager(path)\n\nOpens the video device at "
			"the given path and returns an object that can capture images. The "
			"constructor and all methods except close may raise IOError.", 0, 0, 0,
			0, 0, 0, Video_out_file_manager_methods, 0, 0, 0, 0, 0, 0, 0,
			(initproc)Video_out_file_manager_init
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
	Video_out_file_manager_type.tp_new = PyType_GenericNew;

	if(PyType_Ready(&Device_manager_type) < 0)
		{
			return;
		}
	if(PyType_Ready(&Video_out_manager_type) < 0)
		{
			return;
		}
	if(PyType_Ready(&Video_out_file_manager_type) < 0)
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
	PyModule_AddObject(module, "Video_in_stream_manager", (PyObject *)&Device_manager_type);
	PyModule_AddObject(module, "Video_out_stream_manager", (PyObject *)&Video_out_manager_type);
	PyModule_AddObject(module, "Video_out_file_manager", (PyObject *)&Video_out_file_manager_type);

}
