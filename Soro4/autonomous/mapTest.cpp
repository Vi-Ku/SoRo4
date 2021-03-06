/*
Map processed in MATLAB beforehand so don't have to deal with now.
//#include <jpeglib.h>
//#include <jerror.h>
*/

#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>

using namespace std;

struct dataPoint {
	float lat;
	float lng;
	int gradient;

	//double lat;
	//double lng;
	//double gradient;
};

int main() {
	cout << "line" << endl;
	int colNum = 337;//100 pixel avg 
	int rowNum = 172;
	//int colNum = 3370;//for case of no pixel averaging
	//int rowNum = 1720;

	float lngW = -110.8655;
	float lngE = -110.72;
	float latN = 38.4356;
	float latS = 38.3765;


	ifstream data("mapAvgV1.csv");//100 pixel avg
	//ifstream data("mapNoAvg.csv");//no pixel averaging
	string line;

	//dataPoint map[1720][3370];//no pixel averaging
	dataPoint map[172][337];//100 pixel avg

	vector<vector<string>> parsedCsv;
	while (getline(data, line))
	{
		stringstream lineStream(line);
		string cell;
		vector<string> parsedRow;
		while (getline(lineStream, cell, ','))
		{
			parsedRow.push_back(cell);
		}
		parsedCsv.push_back(parsedRow);
	}

	for (int row = 0; row < rowNum; row++)
	{
		for (int col = 0; col < colNum; col++)
		{
			map[row][col].lat = latN - (((latN - latS) / colNum)*(col + 0.5));
			map[row][col].lng = lngW - (((lngW - lngE) / rowNum)*(row + 0.5));
			map[row][col].gradient = stoi(parsedCsv[row][col]);
		}
	}

	for (int i = 0; i < rowNum; i++)
	{
		//cout << latN - (((latN - latS) / colNum)*(i + 0.5)) << "           ";
		cout << i << "          " << lngW - (((lngW - lngE) / rowNum)*(i + 0.5)) << "           ";
		cout << map[i][0].lat << ", " << map[i][0].lng << ", " << map[i][0].gradient << endl;
	}

}