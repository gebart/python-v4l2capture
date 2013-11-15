
#include "mfvideoin.h"

void *MfVideoIn_Worker_thread(void *arg)
{
	class MfVideoIn *argobj = (class MfVideoIn*) arg;
	argobj->Run();

	return NULL;
}

std::vector<std::string> List_in_devices()
{
	std::vector<std::string> out;
	return out;
}