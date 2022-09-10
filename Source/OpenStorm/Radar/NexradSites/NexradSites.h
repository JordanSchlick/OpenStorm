#include <string.h>

class NexradSites{
public:
	struct Site{
		const char* name = "";
		double latitude = 0.0;
		double longitude = 0.0;
		//elevation above sea level in meters
		double elevation = 0.0;
	};
	static Site sites[];
	static int numberOfSites;
	static inline Site* GetSite(const char* name){
		for(int i = 0; i < numberOfSites; i ++){
			if(strcmp(sites[i].name, name) == 0){
				return &sites[i];
			}
		}
		return NULL;
	}
};