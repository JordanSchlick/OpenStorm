#pragma once

class AsyncTaskRunner{
public:
	// this represents if the task has been canceled. if it is true the code in Task should not modify outside memory
	bool canceled = false;
	
	// is task finished
	bool finished = false;
	
	
	// this function must be overriden and will be run on a separate thread
	virtual void Task() = 0;
	
	// start the task
	void Start();
	// start the task
	void Start(bool autoDelete);
	// cancel the task. the task may continue running in the backround
	void Cancel();
private:
	// if the task will be automaticaly freed upon completion
	bool autoDelete = false;
	
	void InternalTask();
};