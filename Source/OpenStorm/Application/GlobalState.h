#pragma once
#include <string>
#include <functional>
#include <unordered_map>
#include <vector>

class Globe;

class GlobalState{
public:
	
	class Waypoint{
	public:
		double latitude = 0;
		double longitude = 0;
		double altitude = 0;
		int colorR = 255;
		int colorG = 255;
		int colorB = 255;
		std::string name;
		bool enabled = true;
	};
	
	bool inputToggle = false; // toggle mouse input
	bool fade = false; // animate fade
	bool animate = false; // animate the scene
	bool animateCutoff = false; // animate the cotoff
	bool temporalInterpolation = true; // interpolate data over time when animating
	bool spatialInterpolation = true; // interpolate data over time when animating
	bool isMouseCaptured = false; // true if mouse is currently captured
	bool vrMode = false; // true if in vr
	bool pollData = false; // should the data be polled for updates
	bool devShowCacheState = false; // if the state of the cache buffer should be displayed on screen
	bool devShowImGui = false; // if it is safe to show ImGui debuging windows
	bool developmentMode = false; // show development features
	bool vsync = true; // show development features
	bool enableFuzz = true;
	
	float maxFPS = 60.0f; // maximum frames per second
	float animateSpeed = 3.0f; // speed of animation
	float animateCutoffTime = 5.0f; // speed of animation
	float cutoff = 0.0f; // how much of the radar intensity range to hide starting at the lowest
	float opacityMultiplier = 1.0f; // how opaque the radar volume is
	float fadeSpeed = 0.0f; // speed of fade
	float moveSpeed = 300.0f; // speed of movement
	float rotateSpeed = 200.0f; // speed of rotation
	float guiScale = 1.0f; // scale of gui for high dpi displays
	float quality = 0.0f; // a float representing quality with 0 being normal
	float qualityCustomStepSize = 5.0f; // if quality is set to 10 this value is used

	std::vector<Waypoint> locationMarkers = {};
	Globe* globe;
	GlobalState* defaults = NULL;
	
	// register a callback for the given event name. returns a uid to remove callback
	uint64_t RegisterEvent(std::string name, std::function<void(std::string, void*)> callback);
	// remove callback
	void UnregisterEvent(uint64_t uid);
	// emit an event to all registered callbacks
	void EmitEvent(std::string name, std::string stringData, void* extraData);
	void EmitEvent(std::string name);
	
	
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