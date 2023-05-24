#include <string>
#include <map>

class Tar{
public:
	class TarFile {
	public:
		std::string name;
		size_t size = 0;
		size_t offset = 0;
		uint8_t* data = NULL;
		~TarFile(){
			if(data != NULL){
				delete[] data;
			}
		}
	};
	
	
	bool valid = false;
	std::string tarFileName = "";
	std::map<std::string, TarFile> fileMap = {};
	// open a tar file
	Tar(std::string filename);
	// get the contents of a file from the tar, caller must delete TarFile
	TarFile* GetFile(std::string filename);
};