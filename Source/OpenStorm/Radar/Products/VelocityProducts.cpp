#include "VelocityProducts.h"

#include <cmath>
#include <algorithm>
#include <queue>

RadarProductVelocityDealiased::RadarProductVelocityDealiased(){
	volumeType = RadarData::VOLUME_VELOCITY_DEALIASED;
	productType = PRODUCT_DERIVED_VOLUME;
	name = "Radial Velocity Dealiased";
	shortName = "RVD";
	dependencies = {RadarData::VOLUME_VELOCITY};
}


// modulo that always returns positive
inline int modulo(int i, int n) {
	return (i % n + n) % n;
}

class DealiasingAlgorithm {
public:
	class DealiasingGroup{
	public:
		float id = 1;
		int count = 0;
		int iterations = 0;
		int depthReachCount = 0;
		int maxQueueSize = 0;
		
		int startSweep = 0;
		int startTheta = 0;
		int startRadius = 0;
		
		int boundSweepLower = 1000000;
		int boundSweepUpper = -1;
		int boundThetaLower = 1000000;
		int boundThetaUpper = -1;
		int boundRadiusLower = 1000000;
		int boundRadiusUpper = -1;
	};
	struct DealiasingTask{
		int sweep;
		int theta;
		int radius;
		int depth;
		float fromValue;
	};
	// the group of each data point is stored in this volume
	// in the end the groups are combined with the source data to create the final output
	RadarData* vol;
	// source velocity data
	RadarData* src;
	float threashold;
	
	float groupId = 0;
	std::vector<DealiasingGroup> groups = {};
	
	// variables for breadth first search
	// has a location been queued up
	bool* queued;
	// queue of places to search
	std::queue<DealiasingTask> taskQueue = {};
	
	// run a breadth first search to find all elements of the group
	void findGroupExecute(const int sweep, const int theta, const int radius, const float fromValue, DealiasingGroup* group){
		
		group->startSweep = sweep;
		group->startTheta = theta;
		group->startRadius = radius;
		queueFindGroup(sweep, theta, radius, 0, fromValue);
		
		while(!taskQueue.empty()){
			group->maxQueueSize = std::max(group->maxQueueSize, (int)taskQueue.size());
			DealiasingTask nextTask = taskQueue.front();
			taskQueue.pop();
			runFindGroup(nextTask.sweep, nextTask.theta, nextTask.radius, nextTask.depth, nextTask.fromValue, group);
		}
		
		// pad bounds by one
		group->boundSweepLower = std::max(group->boundSweepLower - 1, 0);
		group->boundSweepUpper = std::min(group->boundSweepUpper + 1, vol->sweepBufferCount - 1);
		group->boundThetaLower = std::max(group->boundThetaLower - 1, 0);
		group->boundThetaUpper = std::min(group->boundThetaUpper + 1, vol->thetaBufferCount - 1);
		group->boundRadiusLower = std::max(group->boundRadiusLower - 1, 0);
		group->boundRadiusUpper = std::min(group->boundRadiusUpper + 1, vol->radiusBufferCount - 1);
		
		// clear set queued bools
		for(int sweepClear = group->boundSweepLower; sweepClear <= group->boundSweepUpper; sweepClear++){
			for(int thetaClear = group->boundThetaLower; thetaClear < group->boundThetaUpper; thetaClear++){
				for(int radiusClear = group->boundRadiusLower; radiusClear < group->boundRadiusUpper; radiusClear++){
					int location = sweepClear * vol->sweepBufferSize + (thetaClear + 1) * vol->thetaBufferSize + radiusClear;
					queued[location] = false;
				}
			}
		}
	}
	
	void findAllGroups(){
		for(int sweep = 0; sweep < vol->sweepBufferCount; sweep++){
			for(int theta = 1; theta < vol->thetaBufferCount + 2; theta++){
				float* ray = vol->buffer + (vol->thetaBufferSize * theta + vol->sweepBufferSize * sweep);
				float* raySRC = src->buffer + (vol->thetaBufferSize * theta + vol->sweepBufferSize * sweep);
				if(!vol->rayInfo[sweep * (vol->thetaBufferCount + 2) + theta].interpolated){
					for(int radius = 1; radius < vol->radiusBufferCount; radius++){
						if(raySRC[radius] == 0 || isnan(raySRC[radius])){
							// mark no data spots with -1 in group array
							ray[radius] = -1;
						}
						if (ray[radius] == 0) {
							// not part of a group and has data so create a new group
							DealiasingGroup group = {};
							groupId++;
							group.id = groupId;
							findGroupExecute(sweep, theta, radius, raySRC[radius], &group);
							if(group.count > 100){
								fprintf(stderr, "Group count:%i ittr:%i depthreach:%i queueSize:%i Bounds:s%i-%i,t%i-%i,r%i-%i \n", group.count, group.iterations, group.depthReachCount, group.maxQueueSize, group.boundSweepLower , group.boundSweepUpper , group.boundThetaLower , group.boundThetaUpper , group.boundRadiusLower, group.boundRadiusUpper);
							}
							groups.push_back(group);
						}
					}
				}
			}
		}
		fprintf(stderr, "Groups found: %i\n", (int)groups.size());
	}
private:
	// add task to taskQueue if it has not already been done
	void queueFindGroup(const int sweep, int theta, const int radius, int depth, const float fromValue){
		
		// move to an actual ray
		int rayLocation = sweep * (vol->thetaBufferCount + 2) + (theta + 1);
		theta += vol->rayInfo[rayLocation].closestTheta;
		rayLocation += vol->rayInfo[rayLocation].closestTheta;
		int location = sweep * vol->sweepBufferSize + (theta + 1) * vol->thetaBufferSize + radius;
		// add to queue if it is not already added
		if(!queued[location]){
			queued[location] = true;
			taskQueue.emplace() = {sweep, theta, radius, depth, fromValue};
		}
	}
	
	// try to add a value to a group
	void runFindGroup(const int sweep, int theta, const int radius, int depth, const float fromValue, DealiasingGroup* group){
		depth++;
		group->iterations++;
		//group->id += 0.01f;
		if(depth > 10000){
			group->depthReachCount++;
			return;
		}
		//int theta = thetaOrig;
		int rayLocation = sweep * (vol->thetaBufferCount + 2) + (theta + 1);
		//theta += vol->rayInfo[rayLocation].closestTheta;
		/*if(theta < 0){
			return;
		}*/
		//rayLocation += vol->rayInfo[rayLocation].closestTheta;
		
		// location in both arrays
		int location = sweep * vol->sweepBufferSize + (theta + 1) * vol->thetaBufferSize + radius;
		// value of current location
		float value = src->buffer[location];
		if(vol->buffer[location] != 0.0f || value == 0.0f || abs(value - fromValue) > threashold){
			//if(group->id != vol->buffer[location]){
			//	fprintf(stderr, "%f", vol->buffer[location]);
			//}
			// this space is already part of a group 
			// or there is no value
			// or the difference from the previous iteration is too high to join the group
			return;
		}
		// join the group
		vol->buffer[location] = group->id;
		group->count++;
		group->boundSweepLower = std::min(group->boundSweepLower, sweep);
		group->boundSweepUpper = std::max(group->boundSweepUpper, sweep);
		group->boundThetaLower = std::min(group->boundThetaLower, theta);
		group->boundThetaUpper = std::max(group->boundThetaUpper, theta);
		group->boundRadiusLower = std::min(group->boundRadiusLower, radius);
		group->boundRadiusUpper = std::max(group->boundRadiusUpper, radius);
		
		
		/*if (theta + vol->rayInfo[rayLocation].previousTheta < 0) {
			return;
		}*/
		//if (vol->rayInfo[rayLocation].nextTheta == 0) {
		//	fprintf(stderr, "a");
		//	return;
		//}
		
		// iterate outwards
		
		if(radius + 1 < vol->radiusBufferCount){
			//out
			queueFindGroup(sweep, theta, radius + 1, depth, value);
		}//else{
		//	fprintf(stderr, "a");
		//}
		//right
		queueFindGroup(sweep, theta + vol->rayInfo[rayLocation].nextTheta, radius, depth, value);
		//findGroupDF(sweep, modulo(theta + 1, vol->thetaBufferCount), radius, value, group, depth);
		if(sweep + 1 < vol->sweepBufferCount){
			//up
			queueFindGroup(sweep + 1, theta, radius, depth, value);
		}
		//left
		queueFindGroup(sweep, theta + vol->rayInfo[rayLocation].previousTheta, radius, depth, value);
		//findGroupDF(sweep, modulo(theta - 1, vol->thetaBufferCount), radius, value, group, depth);
		if(radius > 0){
			//in
			queueFindGroup(sweep, theta, radius - 1, depth, value);
		}
		if(sweep > 0){
			//down
			queueFindGroup(sweep - 1, theta, radius, depth, value);
		}
	}
	
	
	
	void findGroupDF(const int sweep, int thetaOrig, const int radius, const float fromValue, DealiasingGroup* group, int depth){
		depth++;
		group->iterations++;
		//group->id += 0.01f;
		if(depth > 360){
			group->depthReachCount++;
			return;
		}
		// move to an actual ray
		int theta = thetaOrig;
		int rayLocation = sweep * (vol->thetaBufferCount + 2) + (theta + 1);
		theta += vol->rayInfo[rayLocation].closestTheta;
		/*if(theta < 0){
			return;
		}*/
		rayLocation += vol->rayInfo[rayLocation].closestTheta;
		
		// location in both arrays
		int location = sweep * vol->sweepBufferSize + (theta + 1) * vol->thetaBufferSize + radius;
		// value of current location
		float value = src->buffer[location];
		if(vol->buffer[location] != 0.0f || value == 0.0f || abs(value - fromValue) > threashold){
			if(group->id != vol->buffer[location]){
				fprintf(stderr, "%f", vol->buffer[location]);
			}
			// this space is already part of a group 
			// or there is no value
			// or the difference from the previous iteration is too high to join the group
			return;
		}
		// join the group
		vol->buffer[location] = group->id;
		group->count++;
		group->boundSweepLower = std::min(group->boundSweepLower, sweep);
		group->boundSweepUpper = std::max(group->boundSweepUpper, sweep);
		group->boundThetaLower = std::min(group->boundThetaLower, theta);
		group->boundThetaUpper = std::max(group->boundThetaUpper, theta);
		group->boundRadiusLower = std::min(group->boundRadiusLower, radius);
		group->boundRadiusUpper = std::max(group->boundRadiusUpper, radius);
		
		
		/*if (theta + vol->rayInfo[rayLocation].previousTheta < 0) {
			return;
		}*/
		if (vol->rayInfo[rayLocation].nextTheta == 0) {
			fprintf(stderr, "a");
			return;
		}
		
		// iterate outwards
		
		if(radius + 1 < vol->radiusBufferCount){
			//out
			findGroupDF(sweep, theta, radius + 1, value, group, depth);
		}else{
			fprintf(stderr, "a");
		}
		//right
		findGroupDF(sweep, theta + vol->rayInfo[rayLocation].nextTheta, radius, value, group, depth);
		findGroupDF(sweep, modulo(theta + 1, vol->thetaBufferCount), radius, value, group, depth);
		if(sweep + 1 < vol->sweepBufferCount){
			//up
			findGroupDF(sweep + 1, theta, radius, value, group, depth);
		}
		//left
		findGroupDF(sweep, theta + vol->rayInfo[rayLocation].previousTheta, radius, value, group, depth);
		//findGroupDF(sweep, modulo(theta - 1, vol->thetaBufferCount), radius, value, group, depth);
		if(radius > 0){
			//in
			findGroupDF(sweep, theta, radius - 1, value, group, depth);
		}
		if(sweep > 0){
			//down
			findGroupDF(sweep - 1, theta, radius, value, group, depth);
		}
	}
};

RadarData* RadarProductVelocityDealiased::deriveVolume(std::map<RadarData::VolumeType, RadarData *> inputProducts){
	RadarData* radarData = new RadarData();
	radarData->CopyFrom(inputProducts[RadarData::VOLUME_VELOCITY]);
	std::fill(radarData->buffer, radarData->buffer + radarData->usedBufferSize, 0.0f);
	
	float valueRange = radarData->stats.maxValue - radarData->stats.minValue;
	float threshold = valueRange * 0.2f;
	
	DealiasingAlgorithm algo = {};
	algo.threashold = threshold;
	algo.vol = radarData;
	algo.src = inputProducts[RadarData::VOLUME_VELOCITY];
	algo.src->Decompress();
	algo.queued = new bool[radarData->fullBufferSize];
	std::fill(algo.queued, algo.queued + radarData->fullBufferSize, false);
	
	//DealiasingAlgorithm::DealiasingGroup group = {};
	//group.id = 100;
	//algo.findGroupDF(0, 0, 0, 0, &group, 0);
	//algo.findGroupExecute(0, 0, 0, 0, &group);
	//fprintf(stderr, "Group count:%i ittr:%i depthreach:%i queueSize:%i Bounds:s%i-%i,t%i-%i,r%i-%i ", group.count, group.iterations, group.depthReachCount, group.maxQueueSize, group.boundSweepLower , group.boundSweepUpper , group.boundThetaLower , group.boundThetaUpper , group.boundRadiusLower, group.boundRadiusUpper);
	
	algo.groupId = 20;
	
	algo.findAllGroups();
	
	radarData->stats.minValue = 0;
	radarData->stats.maxValue = algo.groupId + 1;
	radarData->stats.volumeType = RadarData::VOLUME_UNKNOWN;
	delete algo.queued;
	
	
	//fprintf(stderr, "---- %p %i %i\n", radarData->buffer, radarData->fullBufferSize, inputProducts[RadarData::VOLUME_VELOCITY]->fullBufferSize);
	int len = radarData->fullBufferSize;
	for(int sweep = 0; sweep < radarData->sweepBufferCount; sweep++){
		for(int theta = 1; theta < radarData->thetaBufferCount + 2; theta++){
			float* ray = radarData->buffer + (radarData->thetaBufferSize * theta + radarData->sweepBufferSize * sweep);
			float* raySRC = algo.src->buffer + (radarData->thetaBufferSize * theta + radarData->sweepBufferSize * sweep);
			float* lastRay = ray - radarData->thetaBufferSize;
			float* lastSweepRay = radarData->buffer + (radarData->thetaBufferSize * theta + radarData->sweepBufferSize * sweep - (sweep > 0 ? 1 : 0));
			float offset = 0;
			float last = ray[0];
			for(int radius = 1; radius < 400; radius++){
				if (ray[radius] == 0 && raySRC[radius] != 0 && !radarData->rayInfo[sweep * (radarData->thetaBufferCount + 2) + theta].interpolated) {
					//ray[radius] = radius / 4.0f;
				}
			}
		}
	}
	
	// for(int i = 0; i < len; i++){
	// 	radarData->buffer[i] *= -1;
	// }
	
	/*for(int sweep = 0; sweep < radarData->sweepBufferCount; sweep++){
		for(int theta = 0; theta < radarData->thetaBufferCount + 2; theta++){
			float value = 0;
			float* ray = radarData->buffer + (radarData->thetaBufferSize * theta + radarData->sweepBufferSize * sweep);
			RadarData::RayInfo* rayInfo = &radarData->rayInfo[(radarData->thetaBufferCount + 2) * sweep + theta];
			value = rayInfo->previousTheta;
			//value = rayInfo->nextTheta;
			value = rayInfo->closestTheta;
			value *= 0.05;
			for(int radius = 1; radius < radarData->radiusBufferCount; radius++){
				ray[radius] = value;
			}
		}
	}*/
	return radarData;
}
