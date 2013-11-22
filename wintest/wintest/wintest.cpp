// wintest.cpp : Defines the entry point for the console application.
//

//#include "stdafx.h"
#include "../../mfvideoout.h"
#include <Windows.h>

int main(int argc, char* argv[])
{
	class MfVideoOut mfVideoOut("test");

	while(1)
	{
		Sleep(100);
	}

	return 0;
}

