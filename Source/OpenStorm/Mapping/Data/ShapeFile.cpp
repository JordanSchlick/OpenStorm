#include "ShapeFile.h"

#include <map>

#include "../../Radar/SystemAPI.h"
#include "../../Radar/Globe.h"

void GISObject::Delete(){
	if(geometry != NULL){
		// delete[] geometry;
		// geometry = NULL;
	}
}


inline uint32_t flipEndian(uint32_t input){
	return ((input & 0xff000000) >> 24) | ((input & 0xff0000) >> 8) | ((input & 0xff00) << 8) | ((input & 0xff) << 24);
}
#pragma pack(push, 1)
struct ShapeFileHeader{
	public:
	uint32_t fileCode;
	uint32_t unused[5];
	// total file length in 16-bit words
	uint32_t fileLength;
	uint32_t version;
	uint32_t shapeType;
	double minX;
	double minY;
	double maxX;
	double maxY;
	double minZ;
	double maxZ;
	double minM;
	double maxM;
	void fixEndian(){
		fileCode = flipEndian(fileCode);
		fileLength = flipEndian(fileLength);
	}
};
struct PolygonHeader{
	double minX;
	double minY;
	double maxX;
	double maxY;
	uint32_t numberOfParts;
	uint32_t numberOfPoints;
};
#pragma pack(pop)

bool ReadShapeFile(std::string fileName, std::vector<GISObject>* output, uint8_t groupId)
{
	//return true;
	SystemAPI::FileStats stats = SystemAPI::GetFileStats(fileName);
	if(!stats.exists || stats.size < 100){
		return false;
	}
	//sizeof(ShapeFileHeader)
	FILE* file = fopen(fileName.c_str(), "r");
	if(file == NULL){
		fprintf(stderr, "Could not open shapefile\n");
		return false;
	}
	ShapeFileHeader fileHeader;
	fread(&fileHeader, 1, sizeof(ShapeFileHeader), file);
	fileHeader.fixEndian();
	if(fileHeader.fileCode != 0x0000270a){
		fprintf(stderr, "Invalid shapefile\n");
		fclose(file);
		return false;
	}
	size_t fileSize = std::min((size_t)fileHeader.fileLength * 2, stats.size);
	fprintf(stderr, "Shapefile info %i %i %f %f\n", fileHeader.fileLength * 2, (int)stats.size, (float)fileHeader.minX, (float)fileHeader.maxX);
	
	// exclude header size
	fileSize -= 100;
	// this is 100 bytes to big but that can prevents buffer overruns
	uint8_t* fileContents = new uint8_t[fileSize + 100];
	fread(fileContents, 1, fileSize, file);
	fclose(file);
	
	Globe globe = {};
	size_t index = 0;
	while(index < fileSize - 16){
		uint32_t recordId = flipEndian(*(uint32_t*)(fileContents + index));
		index += 4;
		size_t recordSize = flipEndian(*(uint32_t*)(fileContents + index)) * 2;
		index += 4;
		size_t endIndex = std::min(index + recordSize, fileSize);
		uint32_t shapeType = *(uint32_t*)(fileContents + index);
		index += 4;
		// fprintf(stderr, "Shape %i %i %i\n", recordId, shapeType, (int)recordSize);
		// if(recordId > 100){
		// 	break;
		// }
		if(shapeType == 3/*polyline*/ || shapeType == 5/*polygon*/){
			// get header
			PolygonHeader polyHead = *(PolygonHeader*)(fileContents + index);
			index += sizeof(PolygonHeader);
			// fprintf(stderr, "Polygon %i %i %i\n", polyHead.numberOfParts, polyHead.numberOfPoints, (int)recordSize);
			// if(polyHead.numberOfParts > 1000 || polyHead.numberOfPoints > 10000){
			// 	break;
			// }
			// get parts array
			uint32_t* parts = (uint32_t*)(fileContents + index);
			index += polyHead.numberOfParts * sizeof(uint32_t);
			if(index > endIndex){
				break;
			}
			// get points array
			double* points = (double*)(fileContents + index);
			index += polyHead.numberOfPoints * sizeof(double) * 2;
			if(index > endIndex){
				break;
			}
			// create GISObject
			// output->push_back(GISObject());
			GISObject object = {};//&output[0][output->size() - 1];
			object.groupId = groupId;
			object.geometryCount = polyHead.numberOfPoints * 2 + polyHead.numberOfParts - 1;
			// fprintf(stderr, "%i ", object.geometryCount);
			// float* stuff = new float[object.geometryCount];
			object.geometry = new float[object.geometryCount];
			//object.location = SimpleVector3<float>(globe.GetPointDegrees((polyHead.minY + polyHead.maxY) / 2.0, (polyHead.minX + polyHead.maxX) / 2.0, 0));
			uint32_t geometryIndex = 0;
			uint32_t partIndex = 1;
			SimpleVector3<float> averageLocation = SimpleVector3<float>();
			for(uint32_t i = parts[0]; i < polyHead.numberOfPoints; i++){
				if(partIndex < polyHead.numberOfParts){
					uint32_t part = parts[partIndex];
					if(part == i){
						// add separator between parts
						object.geometry[geometryIndex++] = NAN;
						partIndex++;
					}
				}
				// copy point
				float lat = points[i*2 + 1];
				float lon = points[i*2];
				object.geometry[geometryIndex++] = lat;
				object.geometry[geometryIndex++] = lon;
				averageLocation.Add(globe.GetPointDegrees(lat,lon,0));
			}
			averageLocation.RectangularToSpherical();
			averageLocation.radius() = globe.surfaceRadius;
			averageLocation.SphericalToRectangular();
			object.location = averageLocation;
			if(geometryIndex > object.geometryCount){
				fprintf(stderr, "BAD geometryCount %i %i\n", object.geometryCount, geometryIndex);
				break;
			}
			object.geometryCount = geometryIndex;
			output->push_back(object);
		}
		index = endIndex;
	}
	
	delete[] fileContents;
	
	return true;
}