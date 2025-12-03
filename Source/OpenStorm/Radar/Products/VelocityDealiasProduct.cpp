#include "VelocityDealiasProduct.h"

#include <cmath>
#include <algorithm>
#include <queue>
#include <map>



#define PIF 3.14159265358979323846f

// modulo that always returns positive
inline int modulo(int i, int n) {
	return (i % n + n) % n;
}

// distance in looped space
inline int moduloDistance(int distace, int n) {
	return std::min(modulo(distace, n), modulo(-distace,n));
}


// This is an algorithm I came up with to dealias radar data.
// First it splits the radar data into groups where there are no sudden jumps in velocity.
// This effectively separates aliased sections from normal sections.
// Starting with the smallest group, the groups are then joined into the largest groups around them and their offset from the other group is determined by the boundaries between the groups.
// Because the largest group tends to be the unaliased group, Any connected groups will eventually be joined to it although possibly through multiple other groups.
// Finally once all groups have been joined the true offset of each group can be found by adding its offset to the group it joined.
class DealiasingAlgorithm {
public:
	// class that holds info about an entire group
	class DealiasingGroup{
	public:
		float id = 1;
		
		// parent group that this group joined if positive
		float parentId = -1;
		// normalized offset from parent where 1 is an entire range
		float parentOffset = 0;
		// groups that have joined this group
		std::vector<float> children = {};
		
		// normalized offset where 1 is an entire range
		float offset = 0;
		// offset in the same units as the velocity data
		float offsetUnits = 0;
		
		// count of items in this group and all its child groups
		int totalCount = 0;
		
		// info about group search
		int count = 0;
		int iterations = 0;
		int depthReachCount = 0;
		int maxQueueSize = 0;
		
		// group search starting location
		int startSweep = 0;
		int startTheta = 0;
		int startRadius = 0;
		
		// center of the group
		uint64_t centroidSweep = 0;
		uint64_t centroidRadius = 0;
		// the theta centroid is calculated from 2d to fix wrapping issues
		int centroidTheta = 0;
		float centroid2dX = 0;
		float centroid2dY = 0;
		
		// unoptimized bounds of group
		int boundSweepLower = 1000000;
		int boundSweepUpper = -1;
		int boundThetaLower = 1000000;
		int boundThetaUpper = -1;
		int boundRadiusLower = 1000000;
		int boundRadiusUpper = -1;
		
		void PrintInfo(){
			fprintf(stderr, "Group count:%i ittr:%i depthreach:%i queueSize:%i Bounds:s%i-%i,t%i-%i,r%i-%i \n", this->count, this->iterations, this->depthReachCount, this->maxQueueSize, this->boundSweepLower , this->boundSweepUpper , this->boundThetaLower , this->boundThetaUpper , this->boundRadiusLower, this->boundRadiusUpper);
		}
	};
	// used for a breadth first search fro groups
	struct DealiasingTask{
		int sweep;
		int theta;
		int radius;
		int depth;
		float fromValue;
	};
	// info about neighboring groups used for merging groups
	struct NeighborInfo{
		float count;
		float offset;
	};
	bool verbose = true;
	// the group of each data point is stored in this volume
	// in the end the groups are combined with the source data to create the final output
	RadarData* vol = NULL;
	// source velocity data
	RadarData* src = NULL;
	// threshold for determining group boundary as a multiple of the nyquist velocity
	float threshold = 0;
	// range velocity values from lowest to highest possible in raw data
	// float range = 0;
	
	float groupId = 0;
	std::vector<DealiasingGroup> groups = {};
	std::map<float, DealiasingGroup*> groupsMap = {};
	DealiasingGroup* currentGroup = NULL;
	
	// variables for breadth first search
	// has a location been used/queued up
	bool* used;
	// queue of places to search
	std::queue<DealiasingTask> taskQueue = {};
	
	// run a breadth first search to find all elements of the group
	void FindGroupExecute(const int sweep, const int theta, const int radius, const float fromValue, DealiasingGroup* group){
		
		group->startSweep = sweep;
		group->startTheta = theta;
		group->startRadius = radius;
		
		currentGroup = group;
		
		QueueFindGroup(sweep, theta, radius, 0, fromValue);
		
		while(!taskQueue.empty()){
			group->maxQueueSize = std::max(group->maxQueueSize, (int)taskQueue.size());
			DealiasingTask nextTask = taskQueue.front();
			taskQueue.pop();
			RunFindGroup(nextTask.sweep, nextTask.theta, nextTask.radius, nextTask.depth, nextTask.fromValue);
		}
		
		currentGroup = NULL;
		
		// pad bounds by one
		/*group->boundSweepLower = std::max(group->boundSweepLower - 1, 0);
		group->boundSweepUpper = std::min(group->boundSweepUpper + 1, vol->sweepBufferCount - 1);
		group->boundThetaLower = std::max(group->boundThetaLower - 1, 0);
		group->boundThetaUpper = std::min(group->boundThetaUpper + 1, vol->thetaBufferCount - 1);
		group->boundRadiusLower = std::max(group->boundRadiusLower - 1, 0);
		group->boundRadiusUpper = std::min(group->boundRadiusUpper + 1, vol->radiusBufferCount - 1);*/
		
		// get average centroid
		group->centroidSweep /= group->count;
		group->centroidRadius /= group->count;
		group->centroid2dX /= group->count;
		group->centroid2dY /= group->count;
		group->centroidTheta = modulo((int)(atan2(group->centroid2dY, group->centroid2dX) / 2.0f / PIF * (float)vol->thetaBufferCount), vol->thetaBufferCount);
		// move centroid to actual ray
		group->centroidTheta += vol->rayInfo[group->centroidSweep * (vol->thetaBufferCount + 2) + group->centroidTheta + 1].closestTheta;
		
		//int centroidLocation = group->centroidSweep * vol->sweepBufferSize + (group->centroidTheta + 1) * vol->thetaBufferSize + group->centroidRadius;
		//fprintf(stderr, "centroid %i %i %i\n", (int)group->centroidSweep, group->centroidTheta, (int)group->centroidRadius);
		//vol->buffer[centroidLocation] = 100000;
		
		group->totalCount = group->count;
		
		// clear set used bools
		for(int sweepClear = group->boundSweepLower; sweepClear <= group->boundSweepUpper; sweepClear++){
			for(int thetaClear = group->boundThetaLower; thetaClear <= group->boundThetaUpper; thetaClear++){
				for(int radiusClear = group->boundRadiusLower; radiusClear <= group->boundRadiusUpper; radiusClear++){
					int location = sweepClear * vol->sweepBufferSize + (thetaClear + 1) * vol->thetaBufferSize + radiusClear;
					used[location] = false;
				}
			}
		}
	}
	// search the velocity data and generate groups
	void FindAllGroups(){
		for(int sweep = 0; sweep < vol->sweepBufferCount; sweep++){
			for(int theta = 0; theta < vol->thetaBufferCount; theta++){
				float* ray = vol->buffer + (vol->thetaBufferSize * (theta + 1) + vol->sweepBufferSize * sweep);
				float* raySRC = src->buffer + (vol->thetaBufferSize * (theta + 1) + vol->sweepBufferSize * sweep);
				if(!vol->rayInfo[sweep * (vol->thetaBufferCount + 2) + theta + 1].interpolated){
					for(int radius = 1; radius < vol->radiusBufferCount; radius++){
						float srcValue = raySRC[radius];
						if(srcValue == 0){
							// mark no data spots with -1 in group array
							ray[radius] = -1;
						}
						if(isnan(raySRC[radius])){
							// mark invalid spots with -2 in group array
							ray[radius] = -2;
						}
						if (ray[radius] == 0) {
							// not part of a group and has data so create a new group
							DealiasingGroup group = {};
							groupId++;
							group.id = groupId;
							FindGroupExecute(sweep, theta, radius, raySRC[radius], &group);
							//if(group.count > 100){
							//	group.PrintInfo()
							//}
							if(group.count > 0){
								groups.push_back(group);
							}else{
								group.PrintInfo();
							}
						}
					}
				}
			}
		}
		if(verbose){
			fprintf(stderr, "Groups found: %i\n", (int)groups.size());
		}
	}
	
	// sort groups by size and create a map that points to the elements of the vector
	// as such do not modify the vector after running this function
	void SortGroups(){
		std::sort(groups.begin(), groups.end(), [](DealiasingGroup g1, DealiasingGroup g2){
			// sort by sweep and size
			return /*g1.centroidSweep > g2.centroidSweep ||*/ g1.count < g2.count;
		});
		//groups[groups.size()-1].PrintInfo();
		groupsMap.clear();
		for(auto &group : groups){
			groupsMap[group.id] = &group;
		}
	}
	
	NeighborInfo* neighborInfo;
	std::map<int,bool> isNeighborMap;
	
	float GetNyquistVelocity(int sweep){
		float nyquist = 0;
		nyquist = src->sweepInfo[sweep].nyquistVelocity;
		if(nyquist == 0){
			nyquist = (src->stats.maxValue - src->stats.minValue) / 2.0f;
		}
		return nyquist;
	}
	
	// merge groups into their surrounding groups
	void MergeGroups(){
		neighborInfo = new NeighborInfo[(int)groupId + 1]{};
		int mergedGroups = 0;
		// merge groups group smallest to largest
		//for(int i = 0; i < groups.size() - 1; i++){
		for(int i = groups.size() - 2; i >= 0; i--){
			DealiasingGroup* group = &groups[i];
			if(group->parentId > 0){
				// already merged
				continue;
			}
			currentGroup = group;
			float currentGroupId = group->id;
			float sweepWeight = 0.01;//sqrtf((float)group->count) / (float)group->count * 2.0f;
			for(int sweep = group->boundSweepLower; sweep <= group->boundSweepUpper; sweep++){
				for(int theta = group->boundThetaLower; theta <= group->boundThetaUpper; theta++){
					int rayLocation = sweep * (vol->thetaBufferCount + 2) + (theta + 1);
					if(!vol->rayInfo[rayLocation].interpolated){
						for(int radius = group->boundRadiusLower; radius <= group->boundRadiusUpper; radius++){
							int location = sweep * vol->sweepBufferSize + (theta + 1) * vol->thetaBufferSize + radius;
							if(vol->buffer[location] == currentGroupId){
								float velocityValue = src->buffer[location];
								if(radius + 1 < vol->radiusBufferCount){
									//out
									NeighborCheck(sweep, theta, radius + 1, velocityValue, 1.0f);
								}
								int nextTheta = vol->rayInfo[rayLocation].nextTheta;
								if(moduloDistance(nextTheta, vol->thetaBufferCount) < 10){
									//right/clockwise
									NeighborCheck(sweep, theta + nextTheta, radius, velocityValue, 1.0f);
								}
								if(sweep + 1 < vol->sweepBufferCount){
									//up
									NeighborCheck(sweep + 1, theta, radius, velocityValue, sweepWeight);
								}
								int previousTheta = vol->rayInfo[rayLocation].previousTheta;
								if(moduloDistance(previousTheta, vol->thetaBufferCount) < 10){
									//left/counterclockwise
									NeighborCheck(sweep, theta + previousTheta, radius, velocityValue, 1.0f);
								}
								if(radius > 0){
									//in
									NeighborCheck(sweep, theta, radius - 1, velocityValue, 1.0f);
								}
								if(sweep > 0){
									//down
									NeighborCheck(sweep - 1, theta, radius, velocityValue, sweepWeight);
								}
							}
						}
					}
				}
			}
			
			// find largest group
			// this may not be completely optimal because it does not treat joined groups as the same group when picking the largest group
			float largestGroupId = -1;
			float largestGroupOffset = 0;
			float largestGroupSize = 0;
			for(auto id2 : isNeighborMap){
				int id = id2.first;
				float count = neighborInfo[id].count;
				float offset = neighborInfo[id].offset / (count * GetNyquistVelocity(group->centroidSweep) * 2.0f);
				DealiasingGroup* largestGroup = groupsMap[id];
				bool invalid = false;
				// recurse through parents to make sure of no recursive dependencies
				int maxLoops = 10000;
				while(largestGroup->parentId > 0 && maxLoops > 0){
					largestGroup = groupsMap[largestGroup->parentId];
					if(largestGroup->id == group->id){
						invalid = true;
					}
					maxLoops--;
				}
				if(maxLoops == 0){
					fprintf(stderr, "You fool! You have made the loop run to long.\n");
				}
				float score = count * sqrt(sqrt(largestGroup->totalCount)) * (abs(offset) < 0.2 ? 1.5 : 1) * (abs(offset) < 0.1 ? 1.5 : 1);
				if(score > largestGroupSize){
					// dont join significantly smaller groups
					if(largestGroup->totalCount < group->totalCount * 0.5f){
						invalid = true;
					}
					if(!invalid){
						largestGroupOffset = offset;
						largestGroupId = id;
						largestGroupSize = score;
					}
				}
			}
			
			// join group
			if(largestGroupId > 0){
				DealiasingGroup* largestGroup = groupsMap[largestGroupId];
				largestGroup->children.push_back(group->id);
				largestGroup->totalCount += group->totalCount;
				group->parentId = largestGroupId;
				mergedGroups++;
				// determine offset from parent group from offset of values between groups
				group->parentOffset = 0;
				if(largestGroupOffset > 0.5f){
					group->parentOffset = 1;
				}
				if(largestGroupOffset < -0.5f){
					group->parentOffset = -1;
				}
				// add to total count of all parent groups
				int maxLoops = 10000;
				while(largestGroup->parentId > 0){
					largestGroup = groupsMap[largestGroup->parentId];
					largestGroup->totalCount += group->totalCount;
					maxLoops--;
				}
				if(maxLoops == 0){
					fprintf(stderr, "You fool! You have made the loop run to long.\n");
				}
			}
			
			
			// clear neighbors info for next run
			for(auto id2 : isNeighborMap){
				int id = id2.first;
				neighborInfo[id].count = 0;
				neighborInfo[id].offset = 0;
			}
			isNeighborMap.clear();
			
			// clear set used bools
			for(int sweepClear = group->boundSweepLower; sweepClear <= group->boundSweepUpper; sweepClear++){
				for(int thetaClear = group->boundThetaLower; thetaClear <= group->boundThetaUpper; thetaClear++){
					for(int radiusClear = group->boundRadiusLower; radiusClear <= group->boundRadiusUpper; radiusClear++){
						int location = sweepClear * vol->sweepBufferSize + (thetaClear + 1) * vol->thetaBufferSize + radiusClear;
						used[location] = false;
					}
				}
			}
		}
		delete[] neighborInfo;
		if(verbose){
			fprintf(stderr, "%i out of %i groups merged\n", mergedGroups, (int)groups.size());
		}
		currentGroup = NULL;
	}
	
	// calculate the true offsets of groups based off of the offsets of the groups they have joined
	void CalculateGroupOffsets(){
		for(int i = 0; i < groups.size() - 1; i++){
			DealiasingGroup* group = &groups[i];
			if(group->parentId > 0){
				float offset = 0;
				DealiasingGroup* groupOffset = group;
				int maxLoops = 10000;
				while(groupOffset->parentId > 0 && maxLoops > 0){
					offset += groupOffset->parentOffset;
					groupOffset = groupsMap[groupOffset->parentId];
					maxLoops--;
				}
				if(maxLoops == 0){
					// Unlike the one in RadarCollection I am sure I am actually a fool if this ever prints out.
					// I appear to have already lost a thread forever before I implemented maxLoops.\
					// Probably cyclic group parents
					//fprintf(stderr, "VelocityProducts.cpp(DealiasingAlgorithm::CalculateGroupOffsets) You fool! You have made the loop run to long. %f %f\n", group->id, group->parentId);
					// I am a fool.
					/*int maxLoops2 = 10;
					while(groupOffset->parentId > 0 && maxLoops2 > 0){
						groupOffset = groupsMap[groupOffset->parentId];
						fprintf(stderr, "%f ", groupOffset->id);
						maxLoops2--;
					}
					fprintf(stderr, "\n");
					return;*/
				}
				group->offset = offset;
				group->offsetUnits = offset * GetNyquistVelocity(group->centroidSweep) * 2.0f; // in m/s
			}
		}
	}
	
	// apply the groups and convert vol from group IDs to dealiased velocity data
	void ApplyGroupOffsets(){
		for(int sweep = 0; sweep < vol->sweepBufferCount; sweep++){
			for(int theta = 0; theta < vol->thetaBufferCount; theta++){
				float* ray = vol->buffer + (vol->thetaBufferSize * (theta + 1) + vol->sweepBufferSize * sweep);
				float* raySRC = src->buffer + (vol->thetaBufferSize * (theta + 1) + vol->sweepBufferSize * sweep);
				if(!vol->rayInfo[sweep * (vol->thetaBufferCount + 2) + theta + 1].interpolated){
					for(int radius = 1; radius < vol->radiusBufferCount; radius++){
						if (ray[radius] > 0) {
							DealiasingGroup* group = groupsMap[ray[radius]];
							ray[radius] = raySRC[radius] + group->offsetUnits;
						}else{
							ray[radius] = 0;
						}
					}
				}
			}
		}
	}
	
private:
	// check neighbors of group to determine how to merge
	void NeighborCheck(const int sweep, const int theta, const int radius, const float fromValue, float weight){
		int location = sweep * vol->sweepBufferSize + (theta + 1) * vol->thetaBufferSize + radius;
		float groupValue = vol->buffer[location];
		if(groupValue != currentGroup->id && groupValue > 0 && !used[location]){
			used[location] = true;
			int groupValueInt = (int)(groupValue + 0.5f);
			if(groupValueInt != (int)(groupValue)){
				// should not happen unless there are over one million groups
				fprintf(stderr, "float does not match int");
			}
			float velocityValue = src->buffer[location];
			neighborInfo[groupValueInt].count += weight;
			neighborInfo[groupValueInt].offset += (velocityValue - fromValue) * weight;
			isNeighborMap[groupValueInt] = true;
		}
	}
	// add task to taskQueue if it has not already been done for breadth fist search
	void QueueFindGroup(const int sweep, int thetaOrig, const int radius, int depth, const float fromValue){
		
		// move to an actual ray
		int theta = thetaOrig;
		int rayLocation = sweep * (vol->thetaBufferCount + 2) + (theta + 1);
		theta += vol->rayInfo[rayLocation].closestTheta;
		rayLocation += vol->rayInfo[rayLocation].closestTheta;
		int location = sweep * vol->sweepBufferSize + (theta + 1) * vol->thetaBufferSize + radius;
		/*if(location >= vol->fullBufferSize || location < 0){
			fprintf(stderr, "out of bounds %i  %i %i %i %i\n", location, sweep, theta, radius, thetaOrig);
			return;
		}*/
		// add to queue if it is not already added
		if(!used[location]){
			used[location] = true;
			#if ((defined(_MSVC_LANG) && _MSVC_LANG >= 201703L) || __cplusplus >= 201703L)
			taskQueue.emplace() = {sweep, theta, radius, depth, fromValue};
			#else
			DealiasingTask task;
			task.sweep = sweep;
			task.theta = theta;
			task.radius = radius;
			task.depth = depth;
			task.fromValue = fromValue;
			taskQueue.push(task);
			#endif
			DealiasingGroup* group = currentGroup;
			group->boundSweepLower = std::min(group->boundSweepLower, sweep);
			group->boundSweepUpper = std::max(group->boundSweepUpper, sweep);
			group->boundThetaLower = std::min(group->boundThetaLower, theta);
			group->boundThetaUpper = std::max(group->boundThetaUpper, theta);
			group->boundRadiusLower = std::min(group->boundRadiusLower, radius);
			group->boundRadiusUpper = std::max(group->boundRadiusUpper, radius);
		}
	}
	
	// try to add a value to a group for breadth fist search
	void RunFindGroup(const int sweep, int theta, const int radius, int depth, const float fromValue){
		DealiasingGroup* group = currentGroup;
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
		float velocityValue = src->buffer[location];
		float absoluteThreshold = GetNyquistVelocity(sweep) * threshold;
		if(vol->buffer[location] != 0.0f || velocityValue == 0.0f || isnan(velocityValue) || (fromValue > 0) != (velocityValue > 0) || abs(velocityValue - fromValue) > absoluteThreshold){
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
		group->centroidSweep += sweep;
		group->centroidRadius += radius;
		float angle = ((float)theta + 0.5f) / (float)vol->thetaBufferCount * 2.0f * PIF;
		group->centroid2dX += cos(angle) * (float)radius;
		group->centroid2dY += sin(angle) * (float)radius;
		
		
		/*if (theta + vol->rayInfo[rayLocation].previousTheta < 0) {
			fprintf(stderr, "a");
			return;
		}
		if (theta + vol->rayInfo[rayLocation].nextTheta >= vol->thetaBufferCount) {
			fprintf(stderr, "b");
			return;
		}*/
		//if (vol->rayInfo[rayLocation].nextTheta == 0) {
		//	fprintf(stderr, "a");
		//	return;
		//}
		
		// iterate outwards
		
		if(radius + 1 < vol->radiusBufferCount){
			//out
			QueueFindGroup(sweep, theta, radius + 1, depth, velocityValue);
		}
		int nextTheta = vol->rayInfo[rayLocation].nextTheta;
		if(moduloDistance(nextTheta, vol->thetaBufferCount) < 10){
			//right/clockwise
			QueueFindGroup(sweep, theta + nextTheta, radius, depth, velocityValue);
		}
		//if(sweep + 1 < vol->sweepBufferCount){
			//up
			//QueueFindGroup(sweep + 1, theta, radius, depth, velocityValue);
		//}
		int previousTheta = vol->rayInfo[rayLocation].previousTheta;
		if(moduloDistance(previousTheta, vol->thetaBufferCount) < 10){
			//left/counterclockwise
			QueueFindGroup(sweep, theta + previousTheta, radius, depth, velocityValue);
		}
		if(radius > 0){
			//in
			QueueFindGroup(sweep, theta, radius - 1, depth, velocityValue);
		}
		//if(sweep > 0){
			//down
			//QueueFindGroup(sweep - 1, theta, radius, depth, velocityValue);
		//}
		if(radius + 2 < vol->radiusBufferCount){
			//out farther
			QueueFindGroup(sweep, theta, radius + 2, depth, velocityValue);
		}
		if(radius - 1 > 0){
			//in farther
			QueueFindGroup(sweep, theta, radius - 2, depth, velocityValue);
		}
	}
};

bool RadarProductVelocityDealiased::verbose = false;

RadarProductVelocityDealiased::RadarProductVelocityDealiased(){
	volumeType = RadarData::VOLUME_VELOCITY_DEALIASED;
	productType = PRODUCT_DERIVED_VOLUME;
	name = "Radial Velocity Dealiased";
	shortName = "RVD";
	dependencies = {RadarData::VOLUME_VELOCITY};
}


RadarData* RadarProductVelocityDealiased::deriveVolume(std::map<RadarData::VolumeType, RadarData *> inputProducts){
	RadarData* radarData = new RadarData();
	radarData->CopyFrom(inputProducts[RadarData::VOLUME_VELOCITY]);
	std::fill(radarData->buffer, radarData->buffer + radarData->usedBufferSize, 0.0f);
	
	// float valueRange = radarData->stats.maxValue - radarData->stats.minValue;
	// float threshold = valueRange * 0.2f;
	
	DealiasingAlgorithm algo = {};
	algo.verbose = verbose;
	algo.threshold = 0.3f;
	// algo.range = valueRange;
	algo.vol = radarData;
	algo.src = inputProducts[RadarData::VOLUME_VELOCITY];
	algo.src->Decompress();
	algo.used = new bool[radarData->fullBufferSize];
	std::fill(algo.used, algo.used + radarData->fullBufferSize, false);
	
	//DealiasingAlgorithm::DealiasingGroup group = {};
	//group.id = 100;
	//algo.findGroupDF(0, 0, 0, 0, &group, 0);
	//algo.FindGroupExecute(0, 0, 0, 0, &group);
	//fprintf(stderr, "Group count:%i ittr:%i depthreach:%i queueSize:%i Bounds:s%i-%i,t%i-%i,r%i-%i ", group.count, group.iterations, group.depthReachCount, group.maxQueueSize, group.boundSweepLower , group.boundSweepUpper , group.boundThetaLower , group.boundThetaUpper , group.boundRadiusLower, group.boundRadiusUpper);
	
	algo.groupId = 20;
	
	algo.FindAllGroups();
	
	algo.SortGroups();
	
	algo.MergeGroups();
	algo.MergeGroups();
	algo.MergeGroups();
	
	algo.CalculateGroupOffsets();
	
	algo.ApplyGroupOffsets();
	
	radarData->stats.minValue = 0;
	radarData->stats.maxValue = algo.groupId + 1;
	radarData->stats.volumeType = RadarData::VOLUME_VELOCITY_DEALIASED;
	//radarData->stats.volumeType = RadarData::VOLUME_UNKNOWN;
	delete algo.used;
	
	
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
	radarData->Interpolate();
	return radarData;
}


RadarProductVelocityDealiasedGroupTest::RadarProductVelocityDealiasedGroupTest(RadarData::VolumeType dynamicVolumeType){
	volumeType = dynamicVolumeType;
	productType = PRODUCT_DERIVED_VOLUME;
	development = true;
	name = "RVD Group test";
	shortName = "RVDGT";
	dependencies = {RadarData::VOLUME_VELOCITY};
};
RadarData* RadarProductVelocityDealiasedGroupTest::deriveVolume(std::map<RadarData::VolumeType, RadarData*> inputProducts){
	RadarData* radarData = new RadarData();
	radarData->CopyFrom(inputProducts[RadarData::VOLUME_VELOCITY]);
	std::fill(radarData->buffer, radarData->buffer + radarData->usedBufferSize, 0.0f);
	
	// float valueRange = radarData->stats.maxValue - radarData->stats.minValue;
	// float threshold = valueRange * 0.2f;
	
	DealiasingAlgorithm algo = {};
	algo.threshold = 0.3f;
	// algo.range = valueRange;
	algo.vol = radarData;
	algo.src = inputProducts[RadarData::VOLUME_VELOCITY];
	algo.src->Decompress();
	algo.used = new bool[radarData->fullBufferSize];
	std::fill(algo.used, algo.used + radarData->fullBufferSize, false);
	algo.groupId = 20;

	algo.FindAllGroups();
	
	radarData->stats.minValue = 0;
	radarData->stats.maxValue = algo.groupId + 1;
	radarData->stats.volumeType = RadarData::VOLUME_UNKNOWN;
	delete algo.used;
	
	return radarData;
};
