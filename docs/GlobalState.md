# Global State
Application/GlobalState.h  
GlobalState holds the settings and state of the application. A majority of it will be saved to disk so that settings persist but that is not currently implemented. It also allows broadcasting events.

## Events
Events are named. They can also have some data associated with them. Extra data is shown in parentheses. The data can either be a string or/and void*. Both are always passed but may not be used. Items with no parentheses accept no extra data at all.
* `BackwardStep` - Move to the next file backwards in time
* `ForwardStep` - Move to the next file forwards in time
* `UpdateVolumeParameters` - Used after changing settings involving the volume shader that may require expensive operations on the shader to apply
* `ChangeProduct(unused, RadarData::VolumeType*)` - Change the type of product being displayed
* `LocationMarkersUpdate` - Update location markers from global state
* `GlobeUpdate` - Updated orientation of the globe
* `LoadDirectory(directory to load,Null)` - Load a directory of radar files
* `DevReloadFile` - Reload the current file
* `Teleport(unused, SimpleVector3*)` - Teleport the camera to the game location specified by SimpleVector3
* `ResetBasicSettings` - Reset all simple settings that do not require much setup
* `UpdateEngineSettings` - Update engine settings from global state