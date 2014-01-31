
#ifndef VIDEOIN_H
#define VIDEOIN_H

#include <Python.h>
#include "base.h"
#include <map>
#include <string>

/*typedef struct {
	PyObject_HEAD
	int fd;
	struct buffer *buffers;
	int buffer_count;
} Video_device;*/

class Device_manager_cl{
public:
	PyObject_HEAD
	std::map<std::string, class Base_Video_In *> *threadArgStore;
};
typedef Device_manager_cl Device_manager;

int Device_manager_init(Device_manager *self, PyObject *args,
		PyObject *kwargs);
void Device_manager_dealloc(Device_manager *self);
PyObject *Device_manager_open(Device_manager *self, PyObject *args);
PyObject *Device_manager_set_format(Device_manager *self, PyObject *args);
PyObject *Device_manager_Start(Device_manager *self, PyObject *args);
PyObject *Device_manager_Get_frame(Device_manager *self, PyObject *args);
PyObject *Device_manager_stop(Device_manager *self, PyObject *args);
PyObject *Device_manager_close(Device_manager *self, PyObject *args);
PyObject *Device_manager_list_devices(Device_manager *self);



#endif //VIDEOIN_H

