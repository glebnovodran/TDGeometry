/*
 * TouchDesigner geometry: conversion to Houdini geo/hclassic format
 * Author: Gleb Novodran <novodran@gmail.com>
 */

#include "TDGeometry.hpp"
#include <iostream>
#include <fstream>

using namespace std;

void show_help() {
	cout << "Usage:" << endl;
	cout << "tab2geo <td geo folder>" << endl;
	cout << "OR\n";
	cout << "tab2geo <points file path> <polygons file path>" << endl;
}
void display_stats(const TDGeometry& geo) {
	cout << "Polygons : " << geo.get_poly_num() << endl;
	cout << "Points : " << geo.get_pnt_num() << endl;
	cout << "BBox : " << endl;
	TDGeometry::BBox bbox = geo.bbox();
	cout << bbox.min[0] << " " << bbox.min[1] << " " << bbox.min[2] << endl;
	cout << bbox.max[0] << " " << bbox.max[1] << " " << bbox.max[2] << endl;
}

int main(int argc, char* argv[]) {
	TDGeometry tdgeo;
	if (argc == 2) {
		string inFolder = argv[1];
		if (tdgeo.load(inFolder)) {
			ofstream os("dump.geo");
			os << tdgeo;
			os.close();
			cout << "Saved to dump.geo" << endl;
		} else {
			cout << "Can't load geometry info" << endl;
		}
	} else if (argc == 3) {
		string ptsPath = argv[1];
		string polyPath = argv[2];
		if (tdgeo.load(ptsPath, polyPath)) {
			ofstream os("dump.geo");
			os << tdgeo;
			os.close();
		} else {
			cout << "Can't load geometry info" << endl;
		}
	} else {
		show_help();
	}

	display_stats(tdgeo);

	return 0;
}
