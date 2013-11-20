
#include "mfvideoout.h"

//http://msdn.microsoft.com/en-us/library/windows/desktop/ms700134%28v=vs.85%29.aspx

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
