
#include "v4l2out.h"

int Video_out_manager_init(Video_out_manager *self, PyObject *args,
		PyObject *kwargs)
{
	//self->threadArgStore = new std::map<std::string, class Device_manager_Worker_thread_args*>;
	return 0;
}

void Video_out_manager_dealloc(Video_out_manager *self)
{
	//Stop high level threads
	/*for(std::map<std::string, class Device_manager_Worker_thread_args *>::iterator it = self->threadArgStore->begin(); 
		it != self->threadArgStore->end(); it++)
	{
		PyObject *args = PyTuple_New(1);
		PyTuple_SetItem(args, 0, PyString_FromString(it->first.c_str()));
		Device_manager_stop(self, args);
		Py_DECREF(args);
	}

	delete self->threadArgStore;*/
	self->ob_type->tp_free((PyObject *)self);
}

PyObject *Video_out_manager_open(Video_out_manager *self, PyObject *args)
{


	Py_RETURN_NONE;
}

