#include "AsyncTask.h"


#include "CoreMinimal.h"
#include "Async/AsyncWork.h"

#include <functional>


#ifdef _WIN32
void __cdecl std::_Xbad_function_call() {
	// this symbol is missing in unreal for some reason
}
#endif


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
