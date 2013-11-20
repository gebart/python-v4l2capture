
#include "mfvideoout.h"

//http://msdn.microsoft.com/en-us/library/windows/desktop/ms700134%28v=vs.85%29.aspx


MfVideoOut::MfVideoOut(const char *devName) : Base_Video_Out()
{

}

MfVideoOut::~MfVideoOut()
{

}

void MfVideoOut::SendFrame(const char *imgIn, unsigned imgLen, const char *pxFmt, int width, int height)
{

}

void MfVideoOut::Stop()
{

}

int MfVideoOut::WaitForStop()
{
	return 1;
}

void MfVideoOut::SetOutputSize(int width, int height)
{

}

void MfVideoOut::SetOutputPxFmt(const char *fmt)
{

}

void MfVideoOut::Run()
{

}

//*******************************************************************************

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
