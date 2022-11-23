#include "GlobalState.h"
#include "../Radar/Globe.h"

#include <stdio.h>

uint64_t GlobalState::RegisterEvent(std::string name, std::function<void(std::string, void*)> callback) {
	if(callbacks.find(name) == callbacks.end()){
		callbacks[name] = {};
	}
	uint64_t uid = callbackUid++;
	callbacks[name][uid] = callback;
	callbacksUidNames[uid] = name;
	return uid;
}

void GlobalState::UnregisterEvent(uint64_t uid) {
	if(callbacksUidNames.find(uid) != callbacksUidNames.end()){
		std::string name = callbacksUidNames[uid];
		callbacksUidNames.erase(uid);
		if(callbacks.find(name) != callbacks.end()){
			callbacks[name].erase(uid);
			if(callbacks[name].size() == 0){
				callbacks.erase(name);
			}
		}
	}
}

void GlobalState::EmitEvent(std::string name, std::string stringData, void* extraData) {
	if(callbacks.find(name) != callbacks.end()){
		for(auto callback : callbacks[name]){
			callback.second(stringData, extraData);
		}
	}
}

void GlobalState::EmitEvent(std::string name) {
	EmitEvent(name,"",NULL);
}

void GlobalState::test() {
	fprintf(stderr, "testFloat %f\n", testFloat);
}

GlobalState::GlobalState(){
	#ifdef PLATFORM_ANDROID
	#if PLATFORM_ANDROID
	// temporal interpolation crashes on android
	// the render target seems to be the cause
	temporalInterpolation = false;
	#endif
	#endif
	
	this->defaults = new GlobalState(true);
	globe = new Globe();
	
}

GlobalState::GlobalState(bool doNotInitDefaults){
	// no defaults init
}

GlobalState::~GlobalState(){
	if(this->defaults != NULL){
		delete this->defaults;
	}
}



