#pragma once
#include <string>
#include <functional>
#include <map>
#include <vector>


class GlobalState{
public:
	bool inputToggle = false; // toggle mouse input
	bool fade = false; // animate fade
	bool animate = false; // animate the scene
	bool interpolation = false; // interpolate the scene
	bool isMouseCaptured = false; // true if mouse is currently captured
	
	int maxFPS = 60; // maximum frames per second
	
	float animateSpeed = 3.0f; // speed of animation
	float fadeSpeed = 0.0f; // speed of fade
	float moveSpeed = 300.0f; // speed of movement
	float rotateSpeed = 200.0f; // speed of rotation
	float guiScale = 1.0f; // scale of gui for high dpi displays

	
	// register a callback for the given event name. returns a uid to remove callback
	uint64_t RegisterEvent(std::string name, std::function<void(std::string, void*)> callback);
	// remove callback
	void UnregisterEvent(uint64_t uid);
	// emit an event to all registered callbacks
	void EmitEvent(std::string name, std::string stringData, void* extraData);
	void EmitEvent(std::string name);
	
	
	GlobalState* defaults = NULL;
	
	//Testing
	float testFloat = 1; // test float
	bool testBool = false;
	void test(); // test fuction
	
	GlobalState();
	~GlobalState();
	
private:
	std::unordered_map<std::string, std::unordered_map<uint64_t, std::function<void(std::string, void*)>>> callbacks = {};
	std::unordered_map<uint64_t, std::string> callbacksUidNames = {};
	uint64_t callbackUid = 1;
	// constructor to prevent infinite recursion through defaults
	GlobalState(bool doNotInitDefaults);
};