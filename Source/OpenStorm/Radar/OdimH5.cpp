#include "OdimH5.h"



#ifdef HDF5

#include <map>
#include <mutex>
#include "SparseCompression.h"
#include "SystemAPI.h"


// #define H5_BUILT_AS_DYNAMIC_LIB
// #include "../../../Plugins/UnrealHDF5/Source/UnrealHDF5/Public/hdf5/highfive/H5File.hpp"
// #include "../../../Plugins/UnrealHDF5/Source/UnrealHDF5/Public/hdf5/highfive/H5DataSet.hpp"
// #include "../../../Plugins/UnrealHDF5/Source/UnrealHDF5/Public/hdf5/highfive/H5DataSpace.hpp"
#include "Deps/hdf5/highfive/H5File.hpp"
#include "Deps/hdf5/highfive/H5DataSet.hpp"
#include "Deps/hdf5/highfive/H5DataSpace.hpp"


#define PIF 3.14159265358979323846f

class OdimH5RadarReader::OdimH5Internal{
public:
	HighFive::File* h5File = NULL;
	~OdimH5Internal(){
		if(h5File != NULL){
			delete h5File;
			h5File = NULL;
		}
	}
};

class SweepData{
public:
	HighFive::DataSet dataset;
	int datasetSize = 0;
	HighFive::DataTypeClass datasetTypeClass;
	double elevation = 0;
	int rayCount = 0;
	int binCount = 0;
	float pixelSize = 0;
	float innerDistance = 0;
	float multiplier = 0;
	float offset = 0;
	float sourceNoDataValue = -INFINITY;
	float sourceNoDetectValue = -INFINITY;
	bool discard = false;
	std::vector<float> rayAngles = {};
};

std::string getStringAttribute(HighFive::Group object, std::string attributeName){
	std::string str;
	auto attribute = object.getAttribute(attributeName);
	auto dataType = attribute.getDataType();
	if(dataType.isFixedLenStr()){
		str.resize(dataType.getSize());
		attribute.read(&str[0], dataType);
		str.resize(dataType.getSize() - 1);
	}else if(dataType.isVariableStr()){
		attribute.read(str);
	}
	return str;
}
double getDoubleAttribute(HighFive::Group object, std::string attributeName){
	return object.getAttribute(attributeName).read<double>();
}
int64_t getIntAttribute(HighFive::Group object, std::string attributeName){
	return object.getAttribute(attributeName).read<int64_t>();
}

std::mutex hdf5Lock = {};

#else
class OdimH5RadarReader::OdimH5Internal{

};
#endif


bool OdimH5RadarReader::LoadFile(std::string filename){
	#ifdef HDF5
	
	
	internal = new OdimH5Internal();
	
	try{
		bool failed = false;
		{
			std::lock_guard<std::mutex> lock(hdf5Lock);
			// fprintf(stderr,"begin load ");
			internal->h5File = new HighFive::File(filename, HighFive::File::ReadOnly);
			
			if(!internal->h5File->exist("/dataset1/data1/data")){
				fprintf(stderr, "File has no data\n");
				failed = true;
			}
			// fprintf(stderr,"end load ");
		}
		if(failed){
			UnloadFile();
			return false;
		}
		
	}catch(std::exception e){
		fprintf(stderr, "Exception while loading h5 file: %s\n", e.what());
		UnloadFile();
		return false;
	}
	
	return true;
	
	#else
	fprintf(stderr, "Cannot load file because the HDF5 library is not available\n");
	return false;
	#endif
}


bool OdimH5RadarReader::LoadVolume(RadarData *radarData, RadarData::VolumeType volumeType){
	double benchTime = SystemAPI::CurrentTime();
	radarData->stats = RadarData::Stats();
	std::string dataType = "";
	switch (volumeType){
		case RadarData::VOLUME_REFLECTIVITY:
			dataType = "DBZH";
			break;
		case RadarData::VOLUME_VELOCITY:
			dataType = "VRADH";
			radarData->stats.noDataValue = 0;
			break;
		case RadarData::VOLUME_SPECTRUM_WIDTH:
			dataType = "WRADH";
			break;
		case RadarData::VOLUME_CORELATION_COEFFICIENT:
			dataType = "RHOHV";
			break;
		case RadarData::VOLUME_DIFFERENTIAL_REFLECTIVITY:
			dataType = "ZDR";
			break;
		case RadarData::VOLUME_DIFFERENTIAL_PHASE_SHIFT:
			dataType = "PHIDP";
			break;
		default:
			return false;
			break;
	}
	
	try{
		int maxRadius = 0;
		int maxTheta = 0;
		float minPixelSize = INFINITY;
		std::vector<SweepData> sweeps;
		
		{
			// hdf5 is not thread safe so use a mutex
			std::lock_guard<std::mutex> lock(hdf5Lock);
			if(internal->h5File->exist("where")){
				HighFive::Group whereGroup = internal->h5File->getGroup("where");
				if(whereGroup.hasAttribute("lat") && whereGroup.hasAttribute("lon")){
					radarData->stats.latitude = whereGroup.getAttribute("lat").read<double>();
					radarData->stats.longitude = whereGroup.getAttribute("lon").read<double>();
				}
				if(whereGroup.hasAttribute("height")){
					radarData->stats.altitude = whereGroup.getAttribute("height").read<double>();
				}
			}
			std::map<float, SweepData> sweepsMap;
			for(std::string key : internal->h5File->listObjectNames(HighFive::IndexType::NAME)){
				if(key.substr(0,7) != "dataset" || internal->h5File->getObjectType(key) != HighFive::ObjectType::Group){
					continue;
				}
				HighFive::Group sweepGroup = internal->h5File->getGroup(key);
				if(!sweepGroup.exist("what") || sweepGroup.getObjectType("what") != HighFive::ObjectType::Group){
					fprintf(stderr, "Warning: sweep is missing what group\n");
					continue;
				}
				if(!sweepGroup.exist("how") || sweepGroup.getObjectType("how") != HighFive::ObjectType::Group){
					fprintf(stderr, "Warning: sweep is missing how group\n");
					continue;
				}
				if(!sweepGroup.exist("where") || sweepGroup.getObjectType("where") != HighFive::ObjectType::Group){
					fprintf(stderr, "Warning: sweep is missing how group\n");
					continue;
				}
				HighFive::Group sweepWhat = sweepGroup.getGroup("what");
				HighFive::Group sweepWhere = sweepGroup.getGroup("where");
				HighFive::Group sweepHow = sweepGroup.getGroup("how");
				bool sweepHasAttributes = true;
				// sweepHasAttributes &= sweepWhat.hasAttribute("startdate");
				// sweepHasAttributes &= sweepWhat.hasAttribute("starttime");
				sweepHasAttributes &= sweepWhere.hasAttribute("nbins");
				sweepHasAttributes &= sweepWhere.hasAttribute("nrays");
				sweepHasAttributes &= sweepWhere.hasAttribute("rscale");
				sweepHasAttributes &= sweepWhere.hasAttribute("rstart");
				sweepHasAttributes &= sweepWhere.hasAttribute("elangle");
				sweepHasAttributes &= sweepHow.hasAttribute("startazA");
				sweepHasAttributes &= sweepHow.hasAttribute("stopazA");
				if(!sweepHasAttributes){
					fprintf(stderr, "Warning: sweep is missing attributes\n");
					continue;
				}
				for(std::string key2 : sweepGroup.listObjectNames(HighFive::IndexType::NAME)){
					if(key2.substr(0,4) != "data" || sweepGroup.getObjectType(key2) != HighFive::ObjectType::Group){
						// fprintf(stderr, "%s\n", key2.c_str());
						continue;
					}
					HighFive::Group dataGroup = sweepGroup.getGroup(key2);
					if(!dataGroup.exist("what") || !dataGroup.exist("data") || dataGroup.getObjectType("what") != HighFive::ObjectType::Group || dataGroup.getObjectType("data") != HighFive::ObjectType::Dataset ){
						fprintf(stderr, "Warning: data section is missing groups\n");
						continue;
					}
					HighFive::Group what = dataGroup.getGroup("what");
					if(!what.hasAttribute("gain") || !what.hasAttribute("offset") || !what.hasAttribute("quantity")){
						fprintf(stderr, "Warning: data section is missing what attributes\n");
					}
					// check if quantity is the volume type
					if(getStringAttribute(what, "quantity") != dataType){
						continue;
					}
					// begin extracting data
					float elevationAngle = getDoubleAttribute(sweepWhere, "elangle");
					sweepsMap[elevationAngle] = SweepData();
					SweepData* sweepData = &sweepsMap[elevationAngle];
					HighFive::DataSet dataset = dataGroup.getDataSet("data");
					sweepData->dataset = dataset;
					sweepData->datasetSize = dataset.getElementCount();
					sweepData->datasetTypeClass = dataset.getDataType().getClass();
					sweepData->binCount = getIntAttribute(sweepWhere, "nbins");
					sweepData->rayCount = getIntAttribute(sweepWhere, "nrays");
					maxRadius = std::max(maxRadius, sweepData->binCount);
					maxTheta = std::max(maxTheta, sweepData->rayCount);
					sweepData->pixelSize = getDoubleAttribute(sweepWhere, "rscale");
					sweepData->innerDistance = getDoubleAttribute(sweepWhere, "rstart");
					sweepData->elevation = elevationAngle;
					sweepData->multiplier = getDoubleAttribute(what, "gain");
					sweepData->offset = getDoubleAttribute(what, "offset");
					std::vector<double> startAngles;
					std::vector<double> stopAngles;
					sweepHow.getAttribute("startazA").read(startAngles);
					sweepHow.getAttribute("stopazA").read(stopAngles);
					for(size_t i = 0; i < startAngles.size(); i++){
						sweepData->rayAngles.push_back((startAngles[i] + stopAngles[i]) / 2.0);
					}
					if(what.hasAttribute("nodata")){
						sweepData->sourceNoDataValue = getDoubleAttribute(what, "nodata");
					}
					if(what.hasAttribute("undetect")){
						sweepData->sourceNoDetectValue = getDoubleAttribute(what, "undetect");
					}
				}
			}
			// convert to vector
			std::vector<SweepData> sweepsTmp;
			for(auto &pair : sweepsMap){
				sweepsTmp.push_back(pair.second);
			}
			for(int i = 1; i < sweepsTmp.size() - 1; i++){
				SweepData* sweepBefore = &sweepsTmp[i - 1];
				SweepData* sweep = &sweepsTmp[i];
				SweepData* sweepAfter = &sweepsTmp[i + 1];
				float minSurroundingSweepRadius = std::min(sweepBefore->binCount * sweepBefore->pixelSize, sweepAfter->binCount * sweepAfter->pixelSize);
				float sweepRadius = sweep->binCount * sweep->pixelSize;
				if(sweepRadius < minSurroundingSweepRadius * 0.75){
					// remove sweeps that are less than 3/4 the size of the sweeps around it
					// some radars do a low level sweep with significantly lower range than the rest
					sweep->discard = true;
				}
			}
			for(auto &sweep : sweepsTmp){
				if(!sweep.discard){
					// add to final sweep array
					sweeps.push_back(sweep);
					minPixelSize = std::min(minPixelSize, sweep.pixelSize);
					radarData->stats.innerDistance = sweep.innerDistance;
				}
			}
		}
		
		if(sweeps.size() == 0){
			// no data found for volume type
			fprintf(stderr, "Warning: no data found for volume type\n");
			return false;
		}
		
		// begin filling in RadarData
		
		radarData->stats.volumeType = volumeType;
		radarData->stats.pixelSize = minPixelSize;
		if (radarData->sweepBufferCount == 0) {
			radarData->sweepBufferCount = sweeps.size();
		}
		if (radarData->radiusBufferCount == 0) {
			radarData->radiusBufferCount = maxRadius;
		}
		if (radarData->thetaBufferCount == 0) {
			radarData->thetaBufferCount = maxTheta;
		}
		radarData->thetaBufferSize = radarData->radiusBufferCount;
		radarData->sweepBufferSize = (radarData->thetaBufferCount + 2) * radarData->thetaBufferSize;
		radarData->fullBufferSize = radarData->sweepBufferCount * radarData->sweepBufferSize;
		if (radarData->rayInfo != NULL) {
			delete[] radarData->rayInfo;
		}
		radarData->rayInfo = new RadarData::RayInfo[radarData->sweepBufferCount * (radarData->thetaBufferCount + 2)]{};
		if (radarData->sweepInfo != NULL) {
			delete[] radarData->sweepInfo;
		}
		radarData->sweepInfo = new RadarData::SweepInfo[radarData->sweepBufferCount]{};
		
		// pointer to beginning of a sweep buffer to write to
		float* sweepBuffer = NULL;
		float noDataValue = radarData->stats.noDataValue;
		SparseCompress::CompressorState compressorState = {};
		if(radarData->doCompress){
			// store in compressed form
			compressorState.preCompressedSize = radarData->fullBufferSize / 10;
			compressorState.emptyValue = radarData->stats.noDataValue;
			SparseCompress::compressStart(&compressorState);
			sweepBuffer = new float[radarData->sweepBufferSize];
		}else{
			// store in continuous buffer
			if (radarData->buffer == NULL) {
				radarData->buffer = new float[radarData->fullBufferSize];
				std::fill(radarData->buffer, radarData->buffer + radarData->fullBufferSize, noDataValue);
			}else if(radarData->usedBufferSize > 0){
				std::fill(radarData->buffer, radarData->buffer + radarData->usedBufferSize, noDataValue);
			}
		}
		
		float* inputData = NULL;
		int inputDataSize = 0;
		float minValue = INFINITY;
		float maxValue = -INFINITY;
		int sweepIndex = 0;
		// for (const auto &pair : sweeps) {
		// std::map<float, SweepData>::iterator iterator;
		// for(iterator = sweeps.begin(); iterator != sweeps.end(); ++iterator){
		for(int index = 0; index < sweeps.size(); index++){
			if (sweepIndex >= radarData->sweepBufferCount) {
				// ignore sweeps that wont fit in buffer
				break;
			}
			// const SweepData* sweep = &iterator->second;
			const SweepData* sweep = &sweeps[index];
			radarData->sweepInfo[sweepIndex].actualRayCount = sweep->rayCount;
			radarData->sweepInfo[sweepIndex].elevationAngle = sweep->elevation;
			radarData->sweepInfo[sweepIndex].id = sweepIndex;
			// fprintf(stderr, "%f %i %i %f %f\n", radarData->sweepInfo[sweepIndex].elevationAngle, sweep->rayCount, sweep->binCount, sweep->multiplier, sweep->offset);
			int thetaSize = sweep->rayCount;
			int sweepOffset = sweepIndex * radarData->sweepBufferSize;
			if(radarData->doCompress){
				std::fill(sweepBuffer, sweepBuffer + radarData->sweepBufferSize, noDataValue);
			}else{
				sweepBuffer = radarData->buffer + sweepOffset;
			}
			
			
			if(sweep->datasetSize != inputDataSize){
				if(inputData != NULL){
					delete[] inputData;
				}
				inputDataSize = sweep->datasetSize;
				inputData = new float[inputDataSize];
			}
			if( sweep->datasetTypeClass == HighFive::DataTypeClass::Float){
				std::lock_guard<std::mutex> lock(hdf5Lock);
				sweep->dataset.read(inputData);
			}else{
				{
					std::lock_guard<std::mutex> lock(hdf5Lock);
					sweep->dataset.read((int32_t*)inputData);
					// fprintf(stderr, "extracting %s\n", sweep->dataset.getPath().c_str());
				}
				for(int i = 0; i < inputDataSize; i++){
					// convert ints to floats
					inputData[i] = ((int32_t*)inputData)[i];
				}
			}
			
			
			
			// fill in buffer from rays
			for (int theta = 0; theta < thetaSize; theta++) {
				int maxDataDistance = 0;
					
				// get real angle of ray
				int realTheta = (int)((sweep->rayAngles[theta] * ((float)radarData->thetaBufferCount / 360.0f)) + radarData->thetaBufferCount) % radarData->thetaBufferCount;
				
				RadarData::RayInfo* thisRayInfo = &radarData->rayInfo[(radarData->thetaBufferCount + 2) * sweepIndex + (realTheta + 1)];
				thisRayInfo->actualAngle = sweep->rayAngles[theta];
				thisRayInfo->interpolated = false;
				thisRayInfo->closestTheta = 0;
				float sourceNoDataValue = sweep->sourceNoDataValue;
				float sourceNoDetectValue = sweep->sourceNoDetectValue;
				int thetaIndex = (realTheta + 1) * radarData->thetaBufferSize;
				float multiplier = sweep->multiplier;
				float offset = sweep->offset;
				float* rayBuffer = inputData + (sweep->binCount * theta);
				float scale = minPixelSize / sweep->pixelSize;
				int radiusSize = std::min((int)(sweep->binCount / scale), radarData->radiusBufferCount);
				// fprintf(stderr, "%i %i \n",thetaSize, radiusSize);
				for (int radius = 0; radius < radiusSize; radius++) {
					//int value = (ray->range[radius] - minValue) / divider;
					float value = rayBuffer[(int)(radius * scale)];
					if(value == sourceNoDataValue){
						value = noDataValue;
					}else if(value == sourceNoDetectValue){
						value = noDataValue;
					}else{
						value = value * multiplier + offset;
						if (value == noDataValue){
							// prevent real data from having the same value as invalid data
							value = noDataValue + 0.000001;
						}
						minValue = value != 0 ? (value < minValue ? value : minValue) : minValue;
						maxValue = value > maxValue ? value : maxValue;
						maxDataDistance = std::max(maxDataDistance,radius);
					}
					// value = (rand() % 1000) / 100.0;
					
					sweepBuffer[thetaIndex + radius] = value;
				}
				
				
				float realMaxDistance = radarData->stats.innerDistance + maxDataDistance + 1;
				float realMaxHeight = realMaxDistance*std::sin(PIF / 180.0f * radarData->sweepInfo[sweepIndex].elevationAngle) + 1;
				radarData->stats.boundRadius = std::max(radarData->stats.boundRadius, realMaxDistance);
				radarData->stats.boundUpper = std::max(radarData->stats.boundUpper, realMaxHeight);
				radarData->stats.boundLower = std::min(radarData->stats.boundLower, realMaxHeight);
				//break;
			}
			
			radarData->InterpolateSweep(sweepIndex, sweepBuffer);
			
			
			if(radarData->doCompress){
				SparseCompress::compressValues(&compressorState, sweepBuffer, radarData->sweepBufferSize);
			}
			
			sweepIndex++;
		}
		if (minValue == INFINITY) {
			minValue = 0;
			maxValue = 1;
		}
		radarData->stats.minValue = minValue;
		radarData->stats.maxValue = maxValue;
		
		radarData->usedBufferSize = sweepIndex * radarData->sweepBufferSize;
	
		if(radarData->doCompress){
			delete sweepBuffer;
			if(radarData->bufferCompressed){
				// remove old buffer
				delete[] radarData->bufferCompressed;
			}
			radarData->bufferCompressed = SparseCompress::compressEnd(&compressorState);
			radarData->compressedBufferSize = compressorState.sizeAllocated;
			if(verbose){
				fprintf(stderr, "Compressed size bytes:   %i\n", radarData->compressedBufferSize * 4);
				fprintf(stderr, "Uncompressed size bytes: %i\n", radarData->fullBufferSize * 4);
			}
		}
		
		benchTime = SystemAPI::CurrentTime() - benchTime;
		if(verbose || true){
			fprintf(stderr, "volume loading code took %fs\n", benchTime);
		}
		
		{
			std::lock_guard<std::mutex> lock(hdf5Lock);
			sweeps.clear();
		}
		
		return true;
	}catch(std::exception e){
		fprintf(stderr, "Exception while loading odim volume: %s\n", e.what());
		return false;
	}
}

void OdimH5RadarReader::UnloadFile(){
	if(internal != NULL){
		std::lock_guard<std::mutex> lock(hdf5Lock);
		delete internal;
		internal = NULL;
	}
}

OdimH5RadarReader::~OdimH5RadarReader(){
	UnloadFile();
}
