//@ts-check

var fs = require("fs")

var csv = {
	/**
	 * convert a csv string into an array of objects
	 * @param {string} input 
	 * @returns {Array<Object<string,string>>}
	 */
	decode(input){
		var output = []
		var lines = input.split(/[\r\n]{1,2}/g)
		var columns = lines[0].split(",")
		for(var i = 1; i < lines.length; i++){
			if(lines[i] == ""){
				break
			}
			var line = lines[i].split(",")
			var decodedLine = Object.create(null)
			for(var x = 0; x < columns.length; x++){
				decodedLine[columns[x]] = line[x] || ""
			}
			output.push(decodedLine)
		}
		return output
	},
	/**
	 * convert an array of objects to a csv string
	 * @param {Array<Object<any,any>>} input 
	 * @returns {string}
	 */
	encode(input){
		var output = ""
		var columnsSet = new Set()
		for(let x in input){
			var entry = input[x]
			for(let y in entry){
				columnsSet.add(y)
			}
		}
		var columns = Array.from(columnsSet)
		var comma = false
		for(let y = 0; y < columns.length; y++){
			output += (comma?",":"") + columns[y]
			comma = true
		}
		output += "\n"
		for(let x in input){
			var entry = input[x]
			comma = false
			var line = ""
			for(let y = 0; y < columns.length; y++){
				line += (comma?",":"") + (entry[columns[y]] || "")
				comma = true
			}
			output += line + "\n"
		}
		return output
	}
}


var sites = csv.decode(fs.readFileSync("stations.csv").toString())
var out = `
#include "NexradSites.h"

NexradSites::Site NexradSites::sites[] = {

`

function stringOption(value){
	return '"' + value + '",'
}
function numberOption(value){
	return value + ','
}

var siteCount = 0
for(var x in sites){
	var site = sites[x]
	out += "\t{" + 
	stringOption(site.STATION_ID.split(":")[1]) + 
	numberOption(site.LATITUDE) + 
	numberOption(site.LONGITUDE) + 
	numberOption(parseFloat(site["ELEVATION_(FT)"]) * 0.304800609601) + 
	"},\n"
	siteCount++
}

out += `
};

int NexradSites::numberOfSites = ${siteCount};

`

fs.writeFileSync("NexradSites.cpp", out)