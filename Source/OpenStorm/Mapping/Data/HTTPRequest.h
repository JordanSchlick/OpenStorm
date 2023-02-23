#include <list>
#include <functional>

class HTTPRequest{
public:
	// if the request is done
	bool done = false;
	// if the request is currently running
	bool running = false;
	// if the request has been canceled with delete
	bool canceled = false;
	
	// if the data should be downloaded to memory
	bool toMemory = true;
	// data from http request, populated if successful and toMemory is true
	uint8_t* data = NULL;
	// size of data downloaded
	int dataSize = 0;
	
	
	
	std::function<void(uint8_t*, int)> readyCallback = NULL;
	// set callback, this only works if using with an HTTPPool
	void SetCallback(std::function<void(uint8_t*, int)> callback);
	
	// run the http request
	void Execute();
	
	void Delete();
private:
	// use Delete
	~HTTPRequest();
};


class HTTPPool{
public:
	int maxConnections = 10;
	std::list<HTTPRequest*> currentConnections;
	std::list<HTTPRequest*> pendingConnections;
	// call this function often on the main thread
	void EventLoop();
};