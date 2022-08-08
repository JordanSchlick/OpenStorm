#include "AsyncTask.h"


#include "CoreMinimal.h"
#include "Async/AsyncWork.h"

#include <functional>





class FUnrealAsyncTask : public FNonAbandonableTask {
	//friend class FAutoDeleteAsyncTask<FUnrealAsyncTask>
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



void AsyncTaskRunner::Start() {
	AsyncTaskRunner::Start(false);
}

void AsyncTaskRunner::Start(bool autoDeleteTask) {
	this->autoDelete = autoDeleteTask;
	FAutoDeleteAsyncTask<FUnrealAsyncTask>* task = new FAutoDeleteAsyncTask<FUnrealAsyncTask>([this] {
		InternalTask();
	});
	task->StartBackgroundTask();
}

void AsyncTaskRunner::Cancel() {
	canceled = true;
}

void AsyncTaskRunner::InternalTask() {
	Task();
	finished = true;
	if (autoDelete) {
		delete this;
	}
}
