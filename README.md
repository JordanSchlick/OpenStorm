# OpenStorm Radar

OpenStorm radar is a free and open source 3d radar viewer. It uses Unreal Engine 5 as the base. Using a custom volumetric ray marching shader, entire radar volumes can be displayed.


## Demo Video
[![OpenStorm Demo](docs/img/OpenStorm2.jpg)](https://www.youtube.com/watch?v=9j1-sNnDQwY "OpenStorm Demo")


## Download
You can find a pre-built download [here](https://drive.google.com/drive/folders/1Fl5_HBIH6xGewoTSUabsSh-uv9E9v7WI?usp=sharing)


## Contact
Discord server invite [https://discord.gg/K3aU2hEYvJ](https://discord.gg/K3aU2hEYvJ)


## Building
1. Install Unreal Engine 5 and its dependencies
2. Clone the repo `git clone https://github.com/JordanSchlick/OpenStorm.git`
3. Get submodules `git submodule update --init --recursive`
4. Open the project in unreal engine
5. Hit ctrl+alt+f11 to build project


## Getting data
Currently only bzip2 or uncompressed NEXRAD data is supported  
You can get NEXRAD data from
* https://github.com/JordanSchlick/radar-data to download data in real time.
* https://s3.amazonaws.com/noaa-nexrad-level2/index.html for historical data. Data from 2017 onwards works as is, but earlier data may need to be manually decompressed before it can be read.

To build a standalone build, select package project within the desired platform under the Platforms dropdown.