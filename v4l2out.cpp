
#include "v4l2out.h"

class Video_out
{
public:
	Video_out_manager *self;
	std::string devName;
	int stop;
	int stopped;
	pthread_mutex_t lock;
	int verbose;

	Video_out(const char *devNameIn)
	{
		stop = 0;
		stopped = 1;
		verbose = 1;
		this->devName = devNameIn;
		pthread_mutex_init(&lock, NULL);

	}

	virtual ~Video_out()
	{

		pthread_mutex_destroy(&lock);
	}

protected:



public:
	void Run()
	{
		if(verbose) printf("Thread started: %s\n", this->devName.c_str());
		int running = 1;
		pthread_mutex_lock(&this->lock);
		this->stopped = 0;
		pthread_mutex_unlock(&this->lock);

		try
		{
		while(running)
		{
			printf("Sleep\n");
			usleep(1000000);

			pthread_mutex_lock(&this->lock);
			try
			{

				running = !this->stop;
			}
			catch(std::exception &err)
			{
				if(verbose) printf("An exception has occured: %s\n", err.what());
				running = 0;
			}
			pthread_mutex_unlock(&this->lock);
		}
		}
		catch(std::exception &err)
		{
			if(verbose) printf("An exception has occured: %s\n", err.what());
		}

		if(verbose) printf("Thread stopping\n");
		pthread_mutex_lock(&this->lock);
		this->stopped = 1;
		pthread_mutex_unlock(&this->lock);
	}

	void Stop()
	{
		pthread_mutex_lock(&this->lock);
		this->stop = 1;
		pthread_mutex_unlock(&this->lock);
	}

	int WaitForStop()
	{
		this->Stop();
		while(1)
		{
			pthread_mutex_lock(&this->lock);
			int s = this->stopped;
			pthread_mutex_unlock(&this->lock);

			if(s) return 1;
			usleep(10000);
		}
	}
};

void *Video_out_manager_Worker_thread(void *arg)
{
	class Video_out *argobj = (class Video_out*) arg;
	argobj->Run();

	return NULL;
}

// *****************************************************************

int Video_out_manager_init(Video_out_manager *self, PyObject *args,
		PyObject *kwargs)
{
	self->threads = new std::map<std::string, class Video_out *>;
	return 0;
}

void Video_out_manager_dealloc(Video_out_manager *self)
{
	//Stop high level threads
	for(std::map<std::string, class Video_out *>::iterator it = self->threads->begin(); 
		it != self->threads->end(); it++)
	{



		it->second->Stop();
		it->second->WaitForStop();
	}

	delete self->threads;
	self->threads = NULL;
	self->ob_type->tp_free((PyObject *)self);
}

PyObject *Video_out_manager_open(Video_out_manager *self, PyObject *args)
{
	//Process arguments
	const char *devarg = "/dev/video0";
	if(PyTuple_Size(args) >= 1)
	{
		PyObject *pydevarg = PyTuple_GetItem(args, 0);
		devarg = PyString_AsString(pydevarg);
	}

	//Create worker thread
	pthread_t thread;
	Video_out *threadArgs = new Video_out(devarg);
	(*self->threads)[devarg] = threadArgs;
	threadArgs->self = self;
	pthread_create(&thread, NULL, Video_out_manager_Worker_thread, threadArgs);

	Py_RETURN_NONE;
}

PyObject *Video_out_manager_close(Video_out_manager *self, PyObject *args)
{
	//Process arguments
	const char *devarg = "/dev/video0";
	if(PyTuple_Size(args) >= 1)
	{
		PyObject *pydevarg = PyTuple_GetItem(args, 0);
		devarg = PyString_AsString(pydevarg);
	}

	//Stop worker thread
	std::map<std::string, class Video_out *>::iterator it = self->threads->find(devarg);

	if(it != self->threads->end())
	{
		it->second->Stop();
	}

	Py_RETURN_NONE;
}

