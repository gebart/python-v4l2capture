
#include "namedpipeout.h"

#include <iostream>
#include <string>
using namespace std;

#include <stdio.h>
#include <tchar.h>
#include <strsafe.h>
#define BUFSIZE 1024*1024*10

int ProcessClientMessage(class InstanceConfig &instanceConfig);
VOID GetAnswerToRequest(char *pReply, LPDWORD pchBytes, class InstanceConfig &instanceConfig, class NamedPipeOut *, int frameCount);

class InstanceConfig
{
public:
	std::string rxBuff;
	UINT32 width, height, frameLen;

	InstanceConfig()
	{
		width = 0;
		height = 0;
		frameLen = 0;
	}
};

class ConnectionThreadInfo
{
public:
	HANDLE hPipe;
	class NamedPipeOut *parent;

	ConnectionThreadInfo()
	{
		hPipe = INVALID_HANDLE_VALUE;
		parent = NULL;
	}
};

DWORD WINAPI InstanceThread(LPVOID lpvParam)
// This routine is a thread processing function to read from and reply to a client
// via the open pipe connection passed from the main loop. Note this allows
// the main loop to continue executing, potentially creating more threads of
// of this procedure to run concurrently, depending on the number of incoming
// client connections.
{ 
   HANDLE hHeap      = GetProcessHeap();
   char* pRequest = (char*)HeapAlloc(hHeap, 0, BUFSIZE);
   char* pReply   = (char*)HeapAlloc(hHeap, 0, BUFSIZE);

   DWORD cbBytesRead = 0, cbReplyBytes = 0, cbWritten = 0; 
   BOOL fSuccess = FALSE;
   HANDLE hPipe  = NULL;
   class InstanceConfig instanceConfig;

   // Do some extra error checking since the app will keep running even if this
   // thread fails.

   if (lpvParam == NULL)
   {
       printf( "\nERROR - Pipe Server Failure:\n");
       printf( "   InstanceThread got an unexpected NULL value in lpvParam.\n");
       printf( "   InstanceThread exitting.\n");
       if (pReply != NULL) HeapFree(hHeap, 0, pReply);
       if (pRequest != NULL) HeapFree(hHeap, 0, pRequest);
       return (DWORD)-1;
   }

   if (pRequest == NULL)
   {
       printf( "\nERROR - Pipe Server Failure:\n");
       printf( "   InstanceThread got an unexpected NULL heap allocation.\n");
       printf( "   InstanceThread exitting.\n");
       if (pReply != NULL) HeapFree(hHeap, 0, pReply);
       return (DWORD)-1;
   }

   if (pReply == NULL)
   {
       printf( "\nERROR - Pipe Server Failure:\n");
       printf( "   InstanceThread got an unexpected NULL heap allocation.\n");
       printf( "   InstanceThread exitting.\n");
       if (pRequest != NULL) HeapFree(hHeap, 0, pRequest);
       return (DWORD)-1;
   }

	// Print verbose messages. In production code, this should be for debugging only.
	printf("InstanceThread created, receiving and processing messages.\n");

	// The thread's parameter is a handle to a pipe object instance. 
 
	class ConnectionThreadInfo *info = (class ConnectionThreadInfo *)lpvParam;
	class NamedPipeOut *parent = info->parent;
	hPipe = info->hPipe;
	delete info;

	//Initialise timer
	SYSTEMTIME systime;
	GetSystemTime(&systime);
	FILETIME lastUpdateTime;
	SystemTimeToFileTime(&systime, &lastUpdateTime);
	int frameCount = 0;

// Loop until done reading
   while (1) 
   { 
		SYSTEMTIME systime;
		GetSystemTime(&systime);
		FILETIME fiTime;
		SystemTimeToFileTime(&systime, &fiTime);
		LARGE_INTEGER fiTimeNum;
		fiTimeNum.HighPart = fiTime.dwHighDateTime;
		fiTimeNum.LowPart = fiTime.dwLowDateTime;
		LARGE_INTEGER lastUpdate;
		lastUpdate.HighPart = lastUpdateTime.dwHighDateTime;
		lastUpdate.LowPart = lastUpdateTime.dwLowDateTime;

		LARGE_INTEGER elapse;
		elapse.QuadPart = fiTimeNum.QuadPart - lastUpdate.QuadPart;
		float elapseMs = elapse.LowPart / 10000.f;

   // Read client requests from the pipe. This simplistic code only allows messages
   // up to BUFSIZE characters in length.
      fSuccess = ReadFile( 
         hPipe,        // handle to pipe 
         pRequest,    // buffer to receive data 
         BUFSIZE, // size of buffer 
         &cbBytesRead, // number of bytes read 
         NULL);        // not overlapped I/O 

      if (!fSuccess || cbBytesRead == 0)
      {   
          if (GetLastError() == ERROR_BROKEN_PIPE)
          {
              _tprintf(TEXT("InstanceThread: client disconnected.\n"), GetLastError()); 
          }
          else
          {
              _tprintf(TEXT("InstanceThread ReadFile failed, GLE=%d.\n"), GetLastError()); 
          }
          break;
      }

	  //Process received message
	  instanceConfig.rxBuff.append(pRequest, cbBytesRead);
	  
	  if(elapseMs >= 10.f)
	  {
		  ProcessClientMessage(instanceConfig);

		printf("elapse %f\n", elapseMs);
		// Get response string
		GetAnswerToRequest(pReply, &cbReplyBytes, instanceConfig, parent, frameCount); 
		frameCount++;
 
		// Write the reply to the pipe. 
		fSuccess = WriteFile( 
         hPipe,        // handle to pipe 
         pReply,     // buffer to write from 
         cbReplyBytes, // number of bytes to write 
         &cbWritten,   // number of bytes written 
         NULL);        // not overlapped I/O 

      if (!fSuccess || cbReplyBytes != cbWritten)
      {   
          _tprintf(TEXT("InstanceThread WriteFile failed, GLE=%d.\n"), GetLastError()); 
          break;
      }
	  lastUpdateTime=fiTime;
	  }
	  else
	  {
		  Sleep(1);
	  }
  }

// Flush the pipe to allow the client to read the pipe's contents 
// before disconnecting. Then disconnect the pipe, and close the 
// handle to this pipe instance. 
 
   FlushFileBuffers(hPipe); 
   DisconnectNamedPipe(hPipe); 
   CloseHandle(hPipe); 

   HeapFree(hHeap, 0, pRequest);
   HeapFree(hHeap, 0, pReply);

   printf("InstanceThread exitting.\n");
   return 1;
}

VOID GetAnswerToRequest(char *pReply, LPDWORD pchBytes, class InstanceConfig &instanceConfig, 
		class NamedPipeOut *parent, int frameCount)
{
	if(instanceConfig.frameLen  + 8 < BUFSIZE)
	{
		//Return frame
		UINT32 *numArr = (UINT32 *)pReply;
		numArr[0] = 2;
		numArr[1] = instanceConfig.frameLen;
		unsigned char *imgPix = (unsigned char *)&pReply[8];

		parent->Lock();
		unsigned bytesToCopy = instanceConfig.frameLen;
		//cout << bytesToCopy << "\t" << parent->currentFrameLen << endl;
		if(bytesToCopy > parent->currentFrameLen)
			bytesToCopy = parent->currentFrameLen;

		memcpy(imgPix, parent->currentFrame, bytesToCopy);
		parent->UnLock();

		*pchBytes = 8 + bytesToCopy;
	}
	else
	{
		//Insufficient space in buffer
		UINT32 *numArr = (UINT32 *)pReply;
		numArr[0] = 3;
		numArr[1] = 0;
		*pchBytes = 8;
	}
}

int ProcessClientMessage(class InstanceConfig &instanceConfig)
{
	int count = 0;
	int processing = 1;
	while(processing && instanceConfig.rxBuff.size() > 8)
	{
		UINT32 *wordArray = (UINT32 *)instanceConfig.rxBuff.c_str();
		UINT32 msgType = wordArray[0];
		UINT32 msgLen = wordArray[1];
		if(instanceConfig.rxBuff.size() >= 8+msgLen)
		{
			std::string msg(instanceConfig.rxBuff, 8, msgLen);
			UINT32 *msgArray = (UINT32 *)msg.c_str();
			//printf("%d %d %d\n", rxBuff.size(), msgType, msg.size());

			instanceConfig.rxBuff.assign(instanceConfig.rxBuff, 8+msgLen, instanceConfig.rxBuff.size() - 8 - msgLen);

			if(msgType == 1)
			{
				instanceConfig.width = msgArray[0];
				instanceConfig.height = msgArray[1];
				instanceConfig.frameLen = msgArray[2];
				count ++;
			}

			if(msgType != 1)
			{
				printf("Buffer corruption detected\n");
				return 0;
			}
		}
		else
		{
			processing = 0;
		}
	}

	printf("rx msg count %d\n", count);
	printf("w%d h%d buff%d\n",instanceConfig.width, instanceConfig.height, instanceConfig.frameLen);
	
	return 1;
}


//*******************************************************************************************

NamedPipeOut::NamedPipeOut(const char *devName) : Base_Video_Out()
{
	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	if(hr == RPC_E_CHANGED_MODE)
		throw std::runtime_error("CoInitializeEx failed");

	running = 0;
	currentFrameAlloc = 0;
	currentFrameLen = 0;
	currentFrame = NULL;
	InitializeCriticalSection(&lock);
}

NamedPipeOut::~NamedPipeOut()
{
	CoUninitialize();
}

void NamedPipeOut::SendFrame(const char *imgIn, unsigned imgLen, const char *pxFmt, int width, int height)
{
	cout << "NamedPipeOut::SendFrame" << endl;

	this->Lock();
	if(imgLen > this->currentFrameAlloc || this->currentFrame == NULL)
	{
		delete [] this->currentFrame;
		this->currentFrame = new unsigned char [imgLen];
		this->currentFrameAlloc = imgLen;
	}

	memcpy(this->currentFrame, imgIn, imgLen);
	this->currentFrameLen = imgLen;
	this->UnLock();
}

void NamedPipeOut::Stop()
{
	EnterCriticalSection(&lock);
	this->running = 0;
	LeaveCriticalSection(&lock);
}

int NamedPipeOut::WaitForStop()
{
	return 1;
}

void NamedPipeOut::SetOutputSize(int width, int height)
{

}

void NamedPipeOut::SetOutputPxFmt(const char *fmt)
{

}

void NamedPipeOut::Run()
{
	EnterCriticalSection(&lock);
	this->running = 1;
	int tmpRunning = this->running;
	LeaveCriticalSection(&lock);

	BOOL   fConnected = FALSE; 
	DWORD  dwThreadId = 0; 
	HANDLE hPipe = INVALID_HANDLE_VALUE, hThread = NULL; 
	LPTSTR lpszPipename = TEXT("\\\\.\\pipe\\testpipe"); 
 
// Creates an instance of the named pipe and 
// then waits for a client to connect to it. When the client 
// connects, a thread is created to handle communications 
// with that client, and this loop is free to wait for the
// next client connect request. It is an infinite loop.

   while (tmpRunning) 
   { 
      _tprintf( TEXT("\nPipe Server: Main thread awaiting client connection on %s\n"), lpszPipename);
      hPipe = CreateNamedPipe( 
          lpszPipename,             // pipe name 
          PIPE_ACCESS_DUPLEX,       // read/write access 
          PIPE_TYPE_BYTE |       // message type pipe 
          PIPE_READMODE_BYTE |   // message-read mode 
          PIPE_WAIT,                // blocking mode 
          PIPE_UNLIMITED_INSTANCES, // max. instances  
          BUFSIZE,                  // output buffer size 
          BUFSIZE,                  // input buffer size 
          0,                        // client time-out 
          NULL);                    // default security attribute 

      if (hPipe == INVALID_HANDLE_VALUE) 
      {
          _tprintf(TEXT("CreateNamedPipe failed, GLE=%d.\n"), GetLastError()); 
          return;
      }
 
      // Wait for the client to connect; if it succeeds, 
      // the function returns a nonzero value. If the function
      // returns zero, GetLastError returns ERROR_PIPE_CONNECTED. 
 
      fConnected = ConnectNamedPipe(hPipe, NULL) ? 
         TRUE : (GetLastError() == ERROR_PIPE_CONNECTED); 
 
      if (fConnected) 
      { 
         printf("Client connected, creating a processing thread.\n"); 

		 class ConnectionThreadInfo *info = new class ConnectionThreadInfo();
		 info->parent = this;
		 info->hPipe = hPipe;
      
         // Create a thread for this client. 
         hThread = CreateThread( 
            NULL,              // no security attribute 
            0,                 // default stack size 
            InstanceThread,    // thread proc
            (LPVOID) info,    // thread parameter 
            0,                 // not suspended 
            &dwThreadId);      // returns thread ID

         if (hThread == NULL) 
         {
            _tprintf(TEXT("CreateThread failed, GLE=%d.\n"), GetLastError()); 
            return;
         }
         else CloseHandle(hThread); 
       } 
      else 
        // The client could not connect, so close the pipe. 
         CloseHandle(hPipe); 

	  EnterCriticalSection(&lock);
	  tmpRunning = this->running;
	  LeaveCriticalSection(&lock);
   }
}

void NamedPipeOut::Lock()
{
	EnterCriticalSection(&lock);
}

void NamedPipeOut::UnLock()
{
	LeaveCriticalSection(&lock);
}


//*******************************************************************************

void *NamedPipeOut_Worker_thread(void *arg)
{
	class NamedPipeOut *argobj = (class NamedPipeOut*) arg;
	argobj->Run();

	return NULL;
}

std::vector<std::string> List_out_devices()
{
	std::vector<std::string> out;
	out.push_back("VirtualCamera");
	return out;
}
