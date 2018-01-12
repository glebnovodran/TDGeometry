#include "TDGeometry.hpp"
#include <iostream>
#include <fstream>

using namespace std;

void showHelp() {
	cout << "Usage:" << endl;
	cout << "tab2geo <td geo folder>" << endl;
	cout << "OR\n";
	cout << "tab2geo <points file path> <polygons file path>" << endl;
}

int main(int argc, char* argv[]) {
	cTDGeometry tdgeo;
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
		showHelp();
	}

	return 0;
}
