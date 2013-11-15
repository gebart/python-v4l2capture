
#include "videoin.h"
#ifdef _NT
#include "mfvideoin.h"
#endif
#ifdef _POSIX
#include "v4l2capture.h"
#endif

void Device_manager_dealloc(Device_manager *self)
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

int Device_manager_init(Device_manager *self, PyObject *args,
		PyObject *kwargs)
{
	self->threadArgStore = new std::map<std::string, class Base_Video_In*>;
	return 0;
}

PyObject *Device_manager_open(Device_manager *self, PyObject *args)
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
	#ifdef _POSIX
	Video_in_Manager *threadArgs = new Video_in_Manager(devarg);
	#endif
	#ifdef _NT
	MfVideoIn *threadArgs = new MfVideoIn(devarg);
	#endif

	(*self->threadArgStore)[devarg] = threadArgs;

	#ifdef _POSIX
	pthread_create(&thread, NULL, Video_in_Worker_thread, threadArgs);
	#endif
	#ifdef _NT
	pthread_create(&thread, NULL, MfVideoIn_Worker_thread, threadArgs);
	#endif

	threadArgs->OpenDevice();

	Py_RETURN_NONE;
}

PyObject *Device_manager_set_format(Device_manager *self, PyObject *args)
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

PyObject *Device_manager_Start(Device_manager *self, PyObject *args)
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

PyObject *Device_manager_Get_frame(Device_manager *self, PyObject *args)
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

PyObject *Device_manager_stop(Device_manager *self, PyObject *args)
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

PyObject *Device_manager_close(Device_manager *self, PyObject *args)
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

PyObject *Device_manager_list_devices(Device_manager *self)
{	
	PyObject *out = PyList_New(0);
	std::vector<std::string> devLi = List_in_devices();
	for(unsigned i=0; i<devLi.size(); i++)
	{
		PyList_Append(out, PyString_FromString(devLi[i].c_str()));
	}
	PyList_Sort(out);
	return out;
}
