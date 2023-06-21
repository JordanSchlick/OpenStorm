# OpenStorm Radar

OpenStorm radar is a free and open source 3d radar viewer. It uses Unreal Engine 5 as the base. Using a custom volumetric ray marching shader, entire radar volumes can be displayed.


## Demo Video
[![OpenStorm Demo](docs/img/OpenStorm2.jpg)](https://www.youtube.com/watch?v=9j1-sNnDQwY "OpenStorm Demo")


## Download
You can find a pre-built download [here](https://github.com/JordanSchlick/OpenStorm/releases/latest/)


## Community / Contact
Discord server invite [https://discord.gg/K3aU2hEYvJ](https://discord.gg/K3aU2hEYvJ)

## Features
* Full 3D level 2 radar
* Multithreaded data loading
* No limit on number of files it can browse through
* Load files in real time as they are updated
* Display base radar products (Reflectivity, Radial Velocity, Spectrum Width, Correlation Coefficient, Differential Reflectivity, Differential Phase Shift)
* Display derived products  (De-aliased Velocity, Storm Relative Velocity, Rotation)
* Interpolation in space and time
* Linux and Windows support


## Building
1. Install Unreal Engine 5.2 and its dependencies
2. Clone the repo `git clone https://github.com/JordanSchlick/OpenStorm.git`
3. Get submodules `git submodule update --init --recursive`
4. Open the project in unreal engine
5. Hit ctrl+alt+f11 to build project.  
To build a standalone build, select package project within the desired platform under the Platforms dropdown.

To pull the latest changes to an existing copy of the repo use `git pull --recurse-submodules`

## Getting data
Currently only NEXRAD data is supported  
You can get NEXRAD data from
* https://github.com/JordanSchlick/radar-data to download data in real time.
* https://s3.amazonaws.com/noaa-nexrad-level2/index.html for historical data.


