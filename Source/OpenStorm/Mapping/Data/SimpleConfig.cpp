

#include "SimpleConfig.h"
#include <algorithm>
#include "../../Radar/SystemAPI.h"

SimpleConfig::SimpleConfig(std::string fileName){
	SystemAPI::FileStats stats = SystemAPI::GetFileStats(fileName);
	if(!stats.exists || stats.size == 0){
		return;
	}
	FILE* file = fopen(fileName.c_str(), "r");
	if(file == NULL){
		return;
	}
	char* buffer = new char[stats.size + 1];
	buffer[stats.size] = 0;
	fread(buffer, 1, stats.size, file);
	fclose(file);
	char* line = strtok(buffer, "\n");
	// loop through the string to extract all other tokens
	while( line != NULL ) {
		std::string lineString = std::string(line);
		
		// remove carriage returns
		size_t endPos = lineString.find('\r');
		if(endPos == std::string::npos){
			endPos = lineString.size();
		}
		
		size_t splitPos = lineString.find('=');
		if(splitPos > endPos){
			splitPos = endPos;
		}
		
		if(splitPos < endPos){
			values[lineString.substr(0, splitPos)] = lineString.substr(splitPos + 1, endPos);
		}else{
			values[lineString.substr(0, splitPos)] = "";
		}
		
		line = strtok(NULL, "\n");
	}
}

bool SimpleConfig::HasOption(std::string name){
	return values.count(name) > 0;
}

bool SimpleConfig::GetBool(std::string name, bool defaultValue){
	if(HasOption(name)){
		std::string value = values[name];
		std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c){ return std::tolower(c); });
		bool isFalse = (value == "false" || value == "0" || value == "no");
		bool isTrue = (value == "true" || value == "1" || value == "yes");
		return (isFalse || isTrue) ? isTrue : defaultValue;
	}else{
		return defaultValue;
	}
}

double SimpleConfig::GetDouble(std::string name, double defaultValue){
	if(HasOption(name)){
		std::string value = values[name];
		return std::stod(value);
	}else{
		return defaultValue;
	}
}

float SimpleConfig::GetFloat(std::string name, float defaultValue){
	return (float)GetDouble(name, defaultValue);
}

std::string SimpleConfig::GetString(std::string name, std::string defaultValue){
	if(HasOption(name)){
		return values[name];
	}else{
		return defaultValue;
	}
}