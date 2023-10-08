

#include "RadarDataHolder.h"
#include "RadarCollection.h"
#include "RadarReader.h"
#include "Products/RadarProduct.h"

#include <algorithm>

// asynchronously loads radar data on a separate thread
// this thing is like the Daytona 500, there are no locks to be found
class RadarLoader : public AsyncTaskRunner{
public:
	std::string path;
	RadarDataSettings radarSettings;
	RadarDataHolder* radarHolder;
	uint64_t initialUid = 0;
	RadarLoader(RadarFile fileInfo, RadarDataSettings radarDataSettings, RadarDataHolder* radarDataHolder){
		path = fileInfo.path;
		radarSettings = radarDataSettings;
		radarHolder = radarDataHolder;
		initialUid = RadarDataHolder::CreateUID();
		radarHolder->uid = initialUid;
		radarHolder->loader = this;
		radarHolder->state = RadarDataHolder::State::DataStateLoading;
		Start();
	}
	void Task(){
		
		//radarData->ReadNexrad(path.c_str());
		
		
		int baseProductToLoadCount = 0;
		for(RadarDataHolder::ProductHolder* productHolder : radarHolder->products){
			if(productHolder->product->productType == RadarProduct::PRODUCT_BASE && productHolder->isLoaded == false){
				baseProductToLoadCount++;
			}
		}
		if(baseProductToLoadCount > 0){
			// read in radar file
			// void* nexradData = RadarData::ReadNexradData(path.c_str());
			RadarReader* radarFile = RadarReader::GetLoaderForFile(path);
			bool success = radarFile->LoadFile(path);
			if(!success){
				fprintf(stderr, "RadarDataHolder.cpp(RadarLoader::Task) Failed to load file %s\n", path.c_str());
			}
			if(!canceled && success){
				// load base products
				for(RadarDataHolder::ProductHolder* productHolder : radarHolder->products){
					if(productHolder->product->productType == RadarProduct::PRODUCT_BASE && productHolder->isLoaded == false && !canceled){
						RadarData* radarData = new RadarData();
						radarData->doCompress = !productHolder->isDependency;
						radarData->radiusBufferCount = radarSettings.radiusBufferCount;
						radarData->thetaBufferCount = radarSettings.thetaBufferCount;
						radarData->sweepBufferCount = radarSettings.sweepBufferCount;
						// bool success = radarData->LoadNexradVolume(nexradData, productHolder->product->volumeType);
						success = radarFile->LoadVolume(radarData, productHolder->product->volumeType);
						if(!success){
							fprintf(stderr, "RadarDataHolder.cpp(RadarLoader::Task) VolumeType %i %s is missing from file\n", productHolder->volumeType, productHolder->product->name.c_str());
						}
						if(success && !canceled && initialUid == radarHolder->uid){
							if(productHolder->radarData != NULL){
								delete productHolder->radarData;
							}
							productHolder->isLoaded = true;
							productHolder->radarData = radarData;
						}else{
							delete radarData;
							break;
						}
					}
				}
			}
			// RadarData::FreeNexradData(nexradData);
			radarFile->UnloadFile();
			delete radarFile;
		}
		if(!canceled && initialUid == radarHolder->uid){
			// calculate derived products
			bool deriveProducts = true;
			while(deriveProducts && !canceled && initialUid == radarHolder->uid){
				deriveProducts = false;
				int productCount = radarHolder->products.size();
				for(int i = 0; i < productCount && !canceled; i++){
					auto productHolder = radarHolder->products[i];
					if(!productHolder->isLoaded && productHolder->product->productType == RadarProduct::PRODUCT_DERIVED_VOLUME){
						bool dependenciesMet = true;
						std::map<RadarData::VolumeType, RadarData*> dependencyData = {};
						std::vector<RadarDataHolder::ProductHolder*> dependencyDataHolders = {};
						// check dependencies
						for(auto &type : productHolder->product->dependencies){
							/*if(radarHolder->productsMap.find(type) != radarHolder->productsMap.end()){
								addDependencies = true;
							}*/
							auto dependency = radarHolder->GetProduct(type);
							if(dependency->isLoaded){
								dependencyData[dependency->volumeType] = dependency->radarData;
								dependencyDataHolders.push_back(dependency);
							}else{
								dependenciesMet = false;
								break;
							}
						}
						if(dependenciesMet){
							for(auto &dependencyHolder : dependencyDataHolders){
								// ensure the dependencies do not get freed while in use
								dependencyHolder->StartUsing();
							}
							// derive and save the product and run the loop again
							RadarData* radarData = productHolder->product->deriveVolume(dependencyData);
							if(!canceled && initialUid == radarHolder->uid){
								if(productHolder->radarData != NULL){
									delete productHolder->radarData;
								}
								productHolder->isLoaded = true;
								productHolder->radarData = radarData;
							}else{
								delete radarData;
							}
							for(auto &dependencyHolder : dependencyDataHolders){
								// release dependencies
								dependencyHolder->StopUsing();
							}
							// mark loop to be run again in case something depends on the newly calculated product
							deriveProducts = true;
						}
					}
				}
			}
		}
		
		// discard dependencies
		if(!canceled && initialUid == radarHolder->uid){
			/*radarHolder->products.erase(std::remove_if(begin(radarHolder->products), end(radarHolder->products), [this](RadarDataHolder::ProductHolder* productHolder) { 
				bool doRemove = productHolder->isDependency && !productHolder->isFinal && productHolder->inUse <= 0;
				if(doRemove){
					radarHolder->productsMap.erase(radarHolder->productsMap.find(productHolder->volumeType));
					productHolder->Delete();
				}
				return doRemove; 
			}), end(radarHolder->products));*/
			std::vector<RadarDataHolder::ProductHolder*> productHoldersToKeep = {};
			std::vector<RadarDataHolder::ProductHolder*> productHoldersToDestroy = {};
			int productCount = radarHolder->products.size();
			for(int i = 0; i < productCount && !canceled; i++){
				auto productHolder = radarHolder->products[i];
				bool doRemove = productHolder->isDependency && !productHolder->isFinal && productHolder->inUse <= 0;
				if(doRemove){
					productHoldersToDestroy.push_back(productHolder);
				}else{
					productHoldersToKeep.push_back(productHolder);
				}
			}
			if(!canceled){
				radarHolder->products = productHoldersToKeep;
				for(auto productHolder : productHoldersToDestroy){
					radarHolder->productsMap.erase(productHolder->volumeType);
					productHolder->Delete();
				}
			}
		}
		
		// compress all products to save memory
		if(!canceled && initialUid == radarHolder->uid){
			int productCount = radarHolder->products.size();
			for(int i = 0; i < productCount && !canceled; i++){
				auto productHolder = radarHolder->products[i];
				if(productHolder->radarData != NULL && productHolder->inUse == 0){
					productHolder->StartUsing();
					productHolder->radarData->Compress();
					productHolder->StopUsing();
				}
			}
		}
		
		// fprintf(stderr, "product count %i\n", (int)radarHolder->products.size());
		// for(int i = 0; i < radarHolder->products.size(); i++){
		// 	auto productHolder = radarHolder->products[i];
		// 	fprintf(stderr, "product %s %i %p\n", productHolder->product->name.c_str(), productHolder->isLoaded,  productHolder->radarData);
		// }
		
		// check if main one is loaded and output
		if(!canceled && initialUid == radarHolder->uid){
			if(radarHolder->productsMap.find(radarSettings.volumeType) != radarHolder->productsMap.end()){
				auto productHolder = radarHolder->productsMap[radarSettings.volumeType];
				//fprintf(stderr, "====== %i %i %i %p\n", radarSettings.volumeType, productHolder->volumeType, productHolder->isLoaded, productHolder->radarData);
				if(productHolder->isLoaded == 1 && productHolder->radarData != NULL){
					radarHolder->radarData = productHolder->radarData;
					radarHolder->state = RadarDataHolder::State::DataStateLoaded;
				}else{
					if(productHolder->isLoaded == 1 && productHolder->radarData == NULL){
						fprintf(stderr, "RadarDataHolder.cpp(RadarLoader::Task) VolumeType %i %s is missing RadarData after loading\n", productHolder->volumeType, productHolder->product->name.c_str());
					}
					radarHolder->state = RadarDataHolder::State::DataStateFailed;
				}
			}else{
				radarHolder->state = RadarDataHolder::State::DataStateFailed;
			}
		}
		radarHolder->loader = NULL;
	}
};

void RadarDataHolder::ProductHolder::StartUsing(){
	inUse++;
}
void RadarDataHolder::ProductHolder::StopUsing(){
	inUse--;
	if(shouldFree && inUse <= 0){
		delete this;
	}
}

void RadarDataHolder::ProductHolder::Delete(){
	if(inUse > 0){
		shouldFree = true;
	}else{
		delete this;
	}
}

RadarDataHolder::ProductHolder::~ProductHolder(){
	if(radarData != NULL){
		delete radarData;
		radarData = NULL;
	}
}

uint64_t RadarDataHolder::CreateUID(){
	static uint64_t uid = 1;
	return uid++;
}


RadarDataHolder::RadarDataHolder()
{
	uid = RadarDataHolder::CreateUID();
}

RadarDataHolder::~RadarDataHolder(){
	Unload();
}


RadarDataHolder::ProductHolder* RadarDataHolder::GetProduct(RadarData::VolumeType type){
	if(productsMap.find(type) != productsMap.end()){
		return productsMap[type];
	}else{
		ProductHolder* productHolder = new ProductHolder();
		products.push_back(productHolder);
		productHolder->product = RadarProduct::GetProduct(type);
		if(productHolder->product == NULL){
			fprintf(stderr, "RadarDataHolder.cpp(RadarDataHolder::GetProduct) RadarProduct not found for VolumeType %i, this NEEDS to be fixed by adding a RadarProduct for it\n", type);
			// dummy value to prevent crashing
			productHolder->product = new RadarProduct();
		}
		productHolder->volumeType = type;
		productsMap[type] = productHolder;
		return productHolder;
	}
}

void RadarDataHolder::Load(RadarFile file){
	fileInfo = file;
	Load();
}

void RadarDataHolder::Load(){
	// cancel any existing async tasks
	for(auto task : asyncTasks){
		task->Cancel();
		task->Delete();
	}
	asyncTasks.clear();
	
	if(collection != NULL){
		radarDataSettings = collection->radarDataSettings;
		// add output to requested products
		GetProduct(radarDataSettings.volumeType)->isFinal = true;
	}else{
		// add product if there are no products
		if(products.size() == 0){
			GetProduct(radarDataSettings.volumeType)->isFinal = true;
		}
	}

	
	
	
	
	// add dependencies to requested outputs
	bool addDependencies = true;
	while(addDependencies){
		addDependencies = false;
		int productCount = products.size();
		for(int i = 0; i < productCount; i++){
			auto productHolder = products[i];
			if(!productHolder->isLoaded){
				for(auto &type : productHolder->product->dependencies){
					if(productsMap.find(type) == productsMap.end()){
						// found new missing dependency
						addDependencies = true;
					}
					GetProduct(type)->isDependency = true;
				}
			}
		}
	}
	
	if(fileInfo.path != ""){
		// run asynchronous loading
		asyncTasks.push_back(new RadarLoader(
			fileInfo,
			radarDataSettings,
			this
		));
	}
}

void RadarDataHolder::Unload() {
	if(loader != NULL){
		loader->Cancel();
		loader = NULL;
	}
	for(auto task : asyncTasks){
		task->Cancel();
		task->Delete();
	}
	asyncTasks.clear();
	uid = CreateUID();
	state = RadarDataHolder::State::DataStateUnloaded;
	fileInfo = {};
	/*if(radarData != NULL){
		delete radarData;
		radarData = NULL;
	}*/
	// the radar data will be freed when its product holder is freed
	radarData = NULL;
	for(auto product : products){
		product->Delete();
	}
	products.clear();
	productsMap.clear();
}