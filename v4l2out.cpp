
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
		verbose = 0;
		this->devName = devNameIn;
		pthread_mutex_init(&lock, NULL);

	}

	virtual ~Video_out()
	{


		pthread_mutex_destroy(&lock);
	}


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
			//printf("Sleep\n");
			usleep(1000);

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
	};
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
	//self->threadArgStore = new std::map<std::string, class Video_out*>;
	return 0;
}

void Video_out_manager_dealloc(Video_out_manager *self)
{
	//Stop high level threads
	/*for(std::map<std::string, class Device_manager_Worker_thread_args *>::iterator it = self->threadArgStore->begin(); 
		it != self->threadArgStore->end(); it++)
	{
		PyObject *args = PyTuple_New(1);
		PyTuple_SetItem(args, 0, PyString_FromString(it->first.c_str()));
		Device_manager_stop(self, args);
		Py_DECREF(args);
	}

	delete self->threadArgStore;*/
	self->ob_type->tp_free((PyObject *)self);
}

PyObject *Video_out_manager_open(Video_out_manager *self, PyObject *args)
{


	Py_RETURN_NONE;
}

