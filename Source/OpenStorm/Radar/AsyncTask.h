#pragma once

#include <mutex>

class AsyncTaskRunner{
public:
	// this represents if the task has been canceled.  
	// if it is true the code in Task should not modify outside memory and should exit as soon as possible
	bool canceled = false;
	
	// is task finished
	bool finished = false;
	// if the task is currently running
	bool running = false;
	
	// main lock for changing state of the task
	// acquire this lock when doing anything inside the task that relies on the task not being canceled
	// can be used in the task for general synchronization
	std::mutex lock = std::mutex();
	
	// this function must be overriden and will be run on a separate thread
	virtual void Task() = 0;
	
	// start the task
	void Start();
	// start the task, if autoDelete is true it is not safe to access this object again
	void Start(bool autoDelete);
	// cancel the task. the task may continue running in the background
	void Cancel();
	// delete memory associated with task, it is not safe to access this object again
	void Delete();


	virtual ~AsyncTaskRunner();
private:
	// if the task will be automaticaly freed upon completion
	bool autoDelete = false;
	
	bool clearedForDeletion = false;
	
	void InternalTask();
	
};