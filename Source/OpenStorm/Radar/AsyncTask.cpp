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
	if(running){
		fprintf(stderr, "Error: tried to start AsyncTaskRunner multiple times\n");
		return;
	}
	running = true;
	finished = false;
	canceled = false;
	this->autoDelete = autoDeleteTask;
	FAutoDeleteAsyncTask<FUnrealAsyncTask>* task = new FAutoDeleteAsyncTask<FUnrealAsyncTask>([this] {
		InternalTask();
	});
	task->StartBackgroundTask();
}
#else
#include <thread>
#include <future>
#include <vector>
#include <chrono>
std::vector<std::future<void>> pendingFutures;


void AsyncTaskRunner::Start(bool autoDeleteTask) {
	if(running){
		fprintf(stderr, "Error: tried to start AsyncTaskRunner multiple times\n");
		return;
	}
	running = true;
	finished = false;
	canceled = false;
	this->autoDelete = autoDeleteTask;
	if(0){
		std::future<void> future = std::async(std::launch::async, [this] {
			InternalTask();
		});
		pendingFutures.push_back(std::move(future));
	}else{
		std::thread thread = std::thread([this] {
			InternalTask();
		});
		thread.detach();
	}
}
#endif


void AsyncTaskRunner::Start() {
	AsyncTaskRunner::Start(false);
}

void AsyncTaskRunner::Cancel() {
	canceled = true;
}

void AsyncTaskRunner::Delete() {
	Cancel();
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
	
	#ifdef UE_GAME
	#else
	if(0){
		// clean up futures
		std::vector<std::future<void>> stillPendingFutures;
		//fprintf(stderr, "stillPending before %i\n", (int)pendingFutures.size());
		for(auto& future : pendingFutures){
			if(future.wait_for(std::chrono::seconds(0)) != std::future_status::ready){
				stillPendingFutures.push_back(std::move(future));
			}
		}
		//fprintf(stderr, "stillPendingFutures %i\n", (int)stillPendingFutures.size());
		pendingFutures.clear();
		for(auto& future : stillPendingFutures){
			pendingFutures.push_back(std::move(future));
		}
		//fprintf(stderr, "stillPending after %i\n", (int)pendingFutures.size());
	}
	#endif
	
	
	
	
	if (autoDelete) {
		clearedForDeletion = true;
		delete this;
	}else{
		finished = true;
		running = false;
	}
}

AsyncTaskRunner::~AsyncTaskRunner(){
	if(!clearedForDeletion){
		fprintf(stderr, "WARNING!!!! AsyncTaskRunner was freed improperly, this will likely lead to a crash\n");
		fflush(stderr);
	}
}
