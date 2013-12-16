
#include "mfvideooutfile.h"
#include <mfapi.h>
#include <Mferror.h>

MfVideoOutFile::MfVideoOutFile(const char *devName) : Base_Video_Out()
{
	HRESULT hr = MFStartup(MF_VERSION);
	if(!SUCCEEDED(hr))
		throw std::runtime_error("Media foundation startup failed");

	hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	if(hr == RPC_E_CHANGED_MODE)
		throw std::runtime_error("CoInitializeEx failed");





}

MfVideoOutFile::~MfVideoOutFile()
{
	MFShutdown();

	CoUninitialize();
}

void MfVideoOutFile::SendFrame(const char *imgIn, unsigned imgLen, const char *pxFmt, int width, int height)
{

}

void MfVideoOutFile::Stop()
{

}

int MfVideoOutFile::WaitForStop()
{
	return 1;
}

void MfVideoOutFile::SetOutputSize(int width, int height)
{

}

void MfVideoOutFile::SetOutputPxFmt(const char *fmt)
{

}

void MfVideoOutFile::Run()
{

}

//*******************************************************************************

void *MfVideoOut_File_Worker_thread(void *arg)
{
	class MfVideoOutFile *argobj = (class MfVideoOutFile*) arg;
	argobj->Run();

	return NULL;
}
