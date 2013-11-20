
#include "mfvideoout.h"
#include <mfapi.h>
#include <Mferror.h>

//http://msdn.microsoft.com/en-us/library/windows/desktop/ms700134%28v=vs.85%29.aspx

MfVideoOut::MfVideoOut(const char *devName) : Base_Video_Out()
{
	HRESULT hr = MFStartup(MF_VERSION);
	if(!SUCCEEDED(hr))
		throw std::runtime_error("Media foundation startup failed");

	hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	if(hr == RPC_E_CHANGED_MODE)
		throw std::runtime_error("CoInitializeEx failed");
}

MfVideoOut::~MfVideoOut()
{
	MFShutdown();

	CoUninitialize();
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
