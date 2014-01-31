
#ifndef VIDEOOUTFILE_H
#define VIDEOOUTFILE_H

#include <Python.h>
#include <map>
#include <string>
#include "base.h"

class Video_out_file_manager_cl{
public:
	PyObject_HEAD
	std::map<std::string, class Base_Video_Out *> *threads;
};
typedef Video_out_file_manager_cl Video_out_file_manager;

int Video_out_file_manager_init(Video_out_file_manager *self, PyObject *args,
		PyObject *kwargs);

void Video_out_file_manager_dealloc(Video_out_file_manager *self);

PyObject *Video_out_file_manager_open(Video_out_file_manager *self, PyObject *args);
PyObject *Video_out_file_manager_Send_frame(Video_out_file_manager *self, PyObject *args);
PyObject *Video_out_file_manager_close(Video_out_file_manager *self, PyObject *args);
PyObject *Video_out_file_manager_Set_Frame_Rate(Video_out_file_manager *self, PyObject *args);
PyObject *Video_out_file_manager_Set_Video_Codec(Video_out_file_manager *self, PyObject *args);

#endif //VIDEOOUTFILE_H

