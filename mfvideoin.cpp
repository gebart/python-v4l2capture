
#include "mfvideoin.h"

MfVideoIn::MfVideoIn(const char *devName) : Base_Video_In()
{

}

MfVideoIn::~MfVideoIn()
{

}

void MfVideoIn::Stop()
{
		
}

void MfVideoIn::WaitForStop()
{

}

void MfVideoIn::OpenDevice()
{

}

void MfVideoIn::SetFormat(const char *fmt, int width, int height)
{

}

void MfVideoIn::StartDevice(int buffer_count)
{

}

void MfVideoIn::StopDevice()
{

}

void MfVideoIn::CloseDevice()
{

}

int MfVideoIn::GetFrame(unsigned char **buffOut, class FrameMetaData *metaOut)
{
	return 0;
}

//************************************************************

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