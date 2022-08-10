#include "AsyncTask.h"




#include <functional>




#ifdef UE_GAME
#include "CoreMinimal.h"
#include "Async/AsyncWork.h"
class FUnrealAsyncTask : public FNonAbandonableTask {
	friend class FAutoDeleteAsyncTask<FUnrealAsyncTask>;
public:
	std::function<void()> callback;

	FUnrealAsyncTask(std::function<void()> functionToRun) {
		callback = functionToRun;
	}

	void DoWork() {
		callback();
	}

	FORCEINLINE TStatId GetStatId() const { RETURN_QUICK_DECLARE_CYCLE_STAT(FMyTaskName, STATGROUP_ThreadPoolAsyncTasks); }

	static const TCHAR* Name()
	{
		return TEXT("AsyncTaskHelper");
	}
};


void AsyncTaskRunner::Start(bool autoDeleteTask) {
	this->autoDelete = autoDeleteTask;
	FAutoDeleteAsyncTask<FUnrealAsyncTask>* task = new FAutoDeleteAsyncTask<FUnrealAsyncTask>([this] {
		InternalTask();
	});
	task->StartBackgroundTask();
}

#endif


void AsyncTaskRunner::Start() {
	AsyncTaskRunner::Start(false);
}

void AsyncTaskRunner::Cancel() {
	canceled = true;
}

void AsyncTaskRunner::Delete() {
	if(finished){
		clearedForDeletion = true;
		delete this;
	}else{
		autoDelete = true;
	}
}

void AsyncTaskRunner::InternalTask() {
	if(!canceled){
		Task();
	}
	finished = true;
	if (autoDelete) {
		clearedForDeletion = true;
		delete this;
	}
}

AsyncTaskRunner::~AsyncTaskRunner(){
	if(!clearedForDeletion){
		fprintf(stderr, "WARNING!!!! AsyncTaskRunner was freed improperly, this will likely lead to a crash\n");
		fflush(stderr);
	}
}
