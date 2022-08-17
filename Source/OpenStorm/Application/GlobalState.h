

class GlobalState{
public:
	bool inputToggle = false; // toggle mouse input
	bool fade = false; // animate fade
	bool animate = false; // animate the scene
	bool interpolation = false; // interpolate the scene
	
	int maxFPS = 60; // maximum frames per second
	
	float animateSpeed = 0.0f; // speed of animation
	float fadeSpeed = 0.0f; // speed of fade
	float moveSpeed = 300.0f; // speed of movement
	float rotateSpeed = 200.0f; // speed of rotation

	
	//Testing
	float testFloat = 0; // test float
	bool testBool = false;
	void test(); // test fuction
private:
	
};