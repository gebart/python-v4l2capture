
#include <vector>
#include <pthread.h>
#include <iostream>
#include "videooutfile.h"
#ifdef _NT
#include "mfvideooutfile.h"
#endif
#if _POSIX
//TODO
#endif

int Video_out_file_manager_init(Video_out_file_manager *self, PyObject *args,
		PyObject *kwargs)
{
	self->threads = new std::map<std::string, class Base_Video_Out *>;
	return 0;
}

void Video_out_file_manager_dealloc(Video_out_file_manager *self)
{
	//Stop high level threads
	for(std::map<std::string, class Base_Video_Out *>::iterator it = self->threads->begin(); 
		it != self->threads->end(); it++)
	{
		it->second->Stop();
		it->second->WaitForStop();
	}

	delete self->threads;
	self->threads = NULL;
	self->ob_type->tp_free((PyObject *)self);
}

PyObject *Video_out_file_manager_open(Video_out_file_manager *self, PyObject *args)
{
	std::cout << "Video_out_file_manager_open" << std::endl;

	//Process arguments
	const char *devarg = NULL;
	const char *pxFmtIn = NULL;
	int widthIn = 0;
	int heightIn = 0;

	if(!PyArg_ParseTuple(args, "ssii", &devarg, &pxFmtIn, &widthIn, &heightIn))
	{
		PyErr_Format(PyExc_RuntimeError, "Incorrect arguments to function.");
		Py_RETURN_NONE;
	}

	//Create worker thread
	pthread_t thread;
	MfVideoOutFile *threadArgs = NULL;
	#ifdef _POSIX
	//TODO
	#endif
	#ifdef _NT
	try
	{
		threadArgs = new MfVideoOutFile(devarg);
	}
	catch(std::exception &err)
	{
		PyErr_Format(PyExc_RuntimeError, err.what());
		Py_RETURN_NONE;
	}
	#endif

	#ifdef _NT //TODO Remove ifdef when POSIX approah is established
	(*self->threads)[devarg] = threadArgs;
	try
	{
		threadArgs->SetOutputSize(widthIn, heightIn);
		threadArgs->SetOutputPxFmt(pxFmtIn);
	}
	catch(std::exception &err)
	{
		PyErr_SetString(PyExc_RuntimeError, err.what());
		Py_RETURN_NONE;
	}
	#endif

	#ifdef _POSIX
	//TODO
	#endif
	#ifdef _NT
	pthread_create(&thread, NULL, MfVideoOut_File_Worker_thread, threadArgs);
	#endif

	Py_RETURN_NONE;
}

PyObject *Video_out_file_manager_Send_frame(Video_out_file_manager *self, PyObject *args)
{
	printf("Video_out_file_manager_Send_frame\n");
	//dev = '\\dev\\video0', img, pixel_format, width, height

	//Process arguments
	const char *devarg = NULL;
	const char *imgIn = NULL;
	const char *pxFmtIn = NULL;
	int widthIn = 0;
	int heightIn = 0;

	if(PyObject_Length(args) < 5)
	{
		PyErr_Format(PyExc_RuntimeError, "Too few arguments.");
		Py_RETURN_NONE;
	}

	PyObject *pydev = PyTuple_GetItem(args, 0);
	devarg = PyString_AsString(pydev);

	PyObject *pyimg = PyTuple_GetItem(args, 1);
	imgIn = PyString_AsString(pyimg);
	Py_ssize_t imgLen = PyObject_Length(pyimg);

	PyObject *pyPxFmt = PyTuple_GetItem(args, 2);
	pxFmtIn = PyString_AsString(pyPxFmt);

	PyObject *pyWidth = PyTuple_GetItem(args, 3);
	widthIn = PyInt_AsLong(pyWidth);

	PyObject *pyHeight = PyTuple_GetItem(args, 4);
	heightIn = PyInt_AsLong(pyHeight);

	std::map<std::string, class Base_Video_Out *>::iterator it = self->threads->find(devarg);

	if(it != self->threads->end())
	{
		try
		{
			it->second->SendFrame(imgIn, imgLen, pxFmtIn, widthIn, heightIn);
		}
		catch (std::exception &err)
		{
			PyErr_Format(PyExc_RuntimeError, err.what());
			Py_RETURN_NONE;
		}
	}
	else
	{
		std::cout << "devarg " << devarg << std::endl;
		PyErr_Format(PyExc_RuntimeError, "Device not found.");
		Py_RETURN_NONE;
	}

	Py_RETURN_NONE;
}

PyObject *Video_out_file_manager_close(Video_out_file_manager *self, PyObject *args)
{
	//Process arguments
	const char *devarg = "/dev/video0";
	if(PyTuple_Size(args) >= 1)
	{
		PyObject *pydevarg = PyTuple_GetItem(args, 0);
		devarg = PyString_AsString(pydevarg);
	}

	//Stop worker thread
	std::map<std::string, class Base_Video_Out *>::iterator it = self->threads->find(devarg);

	if(it != self->threads->end())
	{
		try
		{
		it->second->Stop();
		}
		catch(std::exception &err)
		{
			PyErr_Format(PyExc_RuntimeError, err.what());
			Py_RETURN_NONE;
		}

	}

	Py_RETURN_NONE;
}

PyObject *Video_out_file_manager_Set_Frame_Rate(Video_out_file_manager *self, PyObject *args)
{
	//Process arguments
	const char *devarg = NULL;
	int frameRate = 0;

	if(PyObject_Length(args) < 2)
	{
		PyErr_Format(PyExc_RuntimeError, "Too few arguments.");
		Py_RETURN_NONE;
	}

	PyObject *pydev = PyTuple_GetItem(args, 0);
	devarg = PyString_AsString(pydev);

	PyObject *pyFrameRate = PyTuple_GetItem(args, 1);
	frameRate = PyInt_AsLong(pyFrameRate);

	std::map<std::string, class Base_Video_Out *>::iterator it = self->threads->find(devarg);

	if(it != self->threads->end())
	{
		try
		{
		it->second->SetFrameRate(frameRate);
		}
		catch(std::exception &err)
		{
			PyErr_Format(PyExc_RuntimeError, err.what());
			Py_RETURN_NONE;
		}

	}
	else
	{
		PyErr_Format(PyExc_RuntimeError, "Device not found.");
		Py_RETURN_NONE;
	}

	Py_RETURN_NONE;
}

PyObject *Video_out_file_manager_Set_Video_Codec(Video_out_file_manager *self, PyObject *args)
{
	//Process arguments
	const char *devarg = NULL;
	char *videoCodec = NULL;
	int bitRate = 0;

	if(PyObject_Length(args) < 2)
	{
		PyErr_Format(PyExc_RuntimeError, "Too few arguments.");
		Py_RETURN_NONE;
	}

	PyObject *pydev = PyTuple_GetItem(args, 0);
	devarg = PyString_AsString(pydev);

	PyObject *pyVideoCodec = PyTuple_GetItem(args, 1);
	if(pyVideoCodec != Py_None)
		videoCodec = PyString_AsString(pyVideoCodec);
	else
		videoCodec = NULL;

	if(PyObject_Length(args) >= 3)
	{
		PyObject *pyBitRate = PyTuple_GetItem(args, 2);
		bitRate = PyInt_AsLong(pyBitRate);
	}

	std::map<std::string, class Base_Video_Out *>::iterator it = self->threads->find(devarg);

	if(it != self->threads->end())
	{
		try
		{
		it->second->SetVideoCodec(videoCodec, bitRate);
		}
		catch(std::exception &err)
		{
			PyErr_Format(PyExc_RuntimeError, err.what());
			Py_RETURN_NONE;
		}

	}
	else
	{
		PyErr_Format(PyExc_RuntimeError, "Device not found.");
		Py_RETURN_NONE;
	}

	Py_RETURN_NONE;
}

PyObject *Video_out_file_manager_Enable_Real_Time_Frame_Rate(Video_out_file_manager *self, PyObject *args)
{
	//Process arguments
	const char *devarg = NULL;
	int realTimeFrameRate = 0;

	if(PyObject_Length(args) < 2)
	{
		PyErr_Format(PyExc_RuntimeError, "Too few arguments.");
		Py_RETURN_NONE;
	}

	PyObject *pydev = PyTuple_GetItem(args, 0);
	devarg = PyString_AsString(pydev);

	PyObject *pyRealTimeFrameRate = PyTuple_GetItem(args, 1);
	realTimeFrameRate = PyInt_AsLong(pyRealTimeFrameRate);

	std::map<std::string, class Base_Video_Out *>::iterator it = self->threads->find(devarg);

	if(it != self->threads->end())
	{
		try
		{
			it->second->EnableRealTimeFrameRate(realTimeFrameRate);
		}
		catch(std::exception &err)
		{
			PyErr_Format(PyExc_RuntimeError, err.what());
			Py_RETURN_NONE;
		}

	}
	else
	{
		PyErr_Format(PyExc_RuntimeError, "Device not found.");
		Py_RETURN_NONE;
	}

	Py_RETURN_NONE;
}
