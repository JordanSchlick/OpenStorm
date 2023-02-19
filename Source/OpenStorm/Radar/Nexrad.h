#include <string>

namespace Nexrad{
	// recompress an archive using gzip, retuns zero on success
	int RecompressArchive(std::string inFile, std::string outFile);
};