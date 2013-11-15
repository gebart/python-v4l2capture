
#include "mfvideoout.h"

void *MfVideoOut_Worker_thread(void *arg)
{
	class MfVideoOut *argobj = (class MfVideoOut*) arg;
	argobj->Run();

	return NULL;
}

std::vector<std::string> List_out_devices()
{
	std::vector<std::string> out;
	return out;
}
