#pragma once

// TODO: make this thing not full of race conditions

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
	// start the task, if autoDelete is true it is not safe to access this object again
	void Start(bool autoDelete);
	// cancel the task. the task may continue running in the backround
	void Cancel();
	// delete memory associated with task
	void Delete();


	virtual ~AsyncTaskRunner();
private:
	// if the task will be automaticaly freed upon completion
	bool autoDelete = false;
	
	bool clearedForDeletion = false;
	
	void InternalTask();
	
};