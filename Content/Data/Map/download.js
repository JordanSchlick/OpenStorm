let fs = require("fs")
let path = require("path")
let http = require("http")
let https = require("https")

function padInt(num, padToSize){
	let str = "" + num
	while(str.length < padToSize){
		str = "0" + num;
	}
	return str
}

let exists = (fileName) => {
	return new Promise(resolve => fs.access(fileName, fs.constants.F_OK, err => resolve(!err)))
}


async function downloadMap(options){
	await fs.promises.mkdir(path.join(__dirname, options.name), {recursive: true})
	for(let zoom = 0; zoom <= options.maxZoom; zoom++){
		let tileDimCount = 2 ** zoom
		for(let y = 0; y < tileDimCount; y++){
			for(let x = 0; x < tileDimCount; x++){
				let numLength = ("" + tileDimCount).length
				let fileName = path.join(__dirname, options.name, "z" + padInt(zoom, 2) + "y" + padInt(y, numLength) + "x" + padInt(x, numLength))
				switch(options.imageType){
					case "image/jpeg":
						fileName += ".jpg"
						break
					case "image/png":
						fileName += ".png"
						break
				}
				if(await exists(fileName)){
					continue
				}
				//console.log(fileName)
				let requestUrl = options.url.replace("{z}", zoom).replace("{y}", y).replace("{x}", x)
				let data = await new Promise((resolve) => {
					let req = https.request(requestUrl, (res) => {
						let buffers = []
						res.on("data", (data) => {
							buffers.push(data)
						})
						res.on("end", () => {
							resolve(Buffer.concat(buffers))
						})
						res.on("error", () => {
							resolve(undefined)
						})
					})
					req.on("error", () => {
						resolve(undefined)
					})
					req.setHeader("Accept", options.imageType)
					req.setHeader("User-Agent", "OpenStorm/1.0.0")
					req.end()
				})
				if(!data){
					return "Failed to get " + requestUrl
				}else{
					await fs.promises.writeFile(fileName, data)
					console.log("Finished writing " + fileName)
				}
			}
		}
	}
}

downloadMap({
	name: "USGSImageryTopo",
	url: "https://basemap.nationalmap.gov/arcgis/rest/services/USGSImageryTopo/MapServer/tile/{z}/{y}/{x}",
	maxZoom: 5,
	imageType: "image/jpeg"
})

downloadMap({
	name: "USGSImageryOnly",
	url: "https://basemap.nationalmap.gov/arcgis/rest/services/USGSImageryOnly/MapServer/tile/{z}/{y}/{x}",
	maxZoom: 5,
	imageType: "image/jpeg"
})

downloadMap({
	name: "OpenStreetMap",
	url: "https://tile.openstreetmap.org/{z}/{x}/{y}.png",
	maxZoom: 5,
	imageType: "image/png"
})