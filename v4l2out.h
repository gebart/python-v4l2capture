#ifndef __V4L2OUT_H__
#define __V4L2OUT_H__

#include <Python.h>
#include <map>
#include <vector>
#include <string>

class Video_out_manager_cl{
public:
	PyObject_HEAD
	//std::map<std::string, class Device_manager_Worker_thread_args *> *threadArgStore;
};
typedef Video_out_manager_cl Video_out_manager;

int Video_out_manager_init(Video_out_manager *self, PyObject *args,
		PyObject *kwargs);

void Video_out_manager_dealloc(Video_out_manager *self);

PyObject *Video_out_manager_open(Video_out_manager *self, PyObject *args);


// ******************************************************************

static PyMethodDef Video_out_manager_methods[] = {
	{"test", (PyCFunction)Video_out_manager_open, METH_VARARGS,
			 "test(dev = '\\dev\\video0')\n\n"
			 "Open video output."},
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

#endif //__V4L2OUT_H__


