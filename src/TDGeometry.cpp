#include "TDGeometry.hpp"
#include <iostream>
#include <sstream>
#include <fstream>

static const char* PTS_FNAME = "pnt.txt";
static const char* POLY_FNAME = "pol.txt";

cTDGeometry::cTDGeometry() {
	mPnts.clear();
	mPols.clear();
}

bool cTDGeometry::load_pnts(const std::string& pntsPath) {
	using namespace std;

	static struct sPointMap {
		const char* pName;
		float sPoint::* pField;
	} map[] = {
		{ "P(0)", &sPoint::x },
		{ "P(1)", &sPoint::y },
		{ "P(2)", &sPoint::z },
		{ "N(0)", &sPoint::nx },
		{ "N(1)", &sPoint::ny },
		{ "N(2)", &sPoint::nz },
		{ "Cd(0)", &sPoint::r },
		{ "Cd(1)", &sPoint::g },
		{ "Cd(2)", &sPoint::b },
		{ "Cd(3)", &sPoint::a },
		{ "uv(0)", &sPoint::u },
		{ "uv(1)", &sPoint::v },
		{ nullptr, nullptr }
	};

	bool res = true;
	int nrow = 0;
	string row;
	vector<int> columnMap;

	ifstream is(pntsPath);
	if (!is.good()) { return false; }
	mPnts.clear();

	while (getline(is, row)) {
		istringstream ss(row);
		if (nrow == 0) {
			// parse header
			string cname;
			while (ss >> cname) {
				int mapIdx = -1;
				for (int i = 0; map[i].pName; ++i) {
					if (cname == map[i].pName) {
						mapIdx = i;
						break;
					}
				} 
				columnMap.push_back(mapIdx);
			}
		} else {
			int columnIdx = 0;
			float val;
			sPoint pnt = {};
			while (ss >> val) {
				int imap = columnMap[columnIdx];
				if (imap >= 0) {
					pnt.*map[imap].pField = val;
				}
				++columnIdx;
			}
			mPnts.push_back(pnt);
		}
		++nrow;
	}

	return res;
}

bool cTDGeometry::load_pols(const std::string& polsPath) {
	using namespace std;
	const string verticesColName = "vertices";
	bool res = true;
	int nrow = 0;
	string row;
	vector<int> columnMap;
	int vertsIdx = -1;

	ifstream is(polsPath);
	if (!is.good()) { return false; }
	mPols.clear();

	while (getline(is, row)) {
		istringstream ss(row);
		if (nrow == 0) {
			string cname;
			int idx = 0;
			while (ss >> cname) {
				if (cname == verticesColName) {
					vertsIdx = idx;
					break;
				}
				++idx;
			}
			if (vertsIdx == -1) {
				return false;
			}
		} else {
			string column;
			uint32_t val;
			int i = 0;
			sPoly poly = {};
			for (int i = 0; i <= vertsIdx; getline(ss, column, '\t'), ++i);

			istringstream cs(column);
			while (cs >> val) {
				poly.ipnt[poly.nvtx] = val;
				poly.nvtx++;
				if (poly.nvtx > MAX_POLY_VERTS) {
					cout << "Warning: Polygon #" << nrow + 1 << " is an n-gon. It was clipped to a quad" << endl;
					break;
				}
			}
			mPols.push_back(poly);
		}
		++nrow;
	}

	return true;
}

bool cTDGeometry::load(const std::string& folder) {
	using namespace std;
	string pntsPath = folder + "/"+ PTS_FNAME;
	string polyPath = folder + "/" + POLY_FNAME;

	return load(pntsPath, polyPath);
}

bool cTDGeometry::load(const std::string& pntsPath, const std::string& polsPath) {
	using namespace std;
	bool res = load_pnts(pntsPath);
	if (res) {
		res = load_pols(polsPath);
		if (!res) {
			cout << "Can't load polygons from " << polsPath << endl;
		}
	} else {
		cout << "Can't load points from "<< pntsPath << endl;
	}
	return res;
}

bool cTDGeometry::dump_geo(std::ostream& os) const {
	using namespace std;

	if (!os.good()) { return false; }

	os << "PGEOMETRY V5" << endl;
	os << "NPoints " << get_pnt_num() << " NPrims " << get_poly_num() << endl;
	os << "NPointGroups 0 NPrimGroups 0" << endl;
	os << "NPointAttrib 3 NVertexAttrib 0 NPrimAttrib 0 NAttrib 0" << endl;
	os << "PointAttrib" << endl;
	os << "N 3 vector 0 0 0" << endl;
	os << "uv 3 float 0 0 0" << endl;
	os << "Cd 3 float 1 1 1" << endl;

	for (auto& pt : mPnts) {
		os << pt.x << " " << pt.y << " " << pt.z << " 1";
		os << " (";
		os << pt.nx << " " << pt.ny << " " << pt.nz << "  ";
		os << pt.u << " " << pt.v << " 1  ";
		os << pt.r << " " << pt.g << " " << pt.b;
		os << ")" << endl;
	}

	os << "Run "<< get_poly_num() <<" Poly" << endl;
	for (auto& poly : mPols) {
		os << " " << poly.nvtx << " <";
		for (int idx = poly.nvtx -1; idx >= 0; --idx) {
			os << " " << poly.ipnt[idx];
		}
		os << endl;
	}

	os << "beginExtra" << endl;
	os << "endExtra" << endl;

	return true;
}
