/*
 * TouchDesigner geometry: data loading and conversion
 * Author: Gleb Novodran <novodran@gmail.com>
 */

#include <ostream>
#include <vector>

class TDGeometry {
public:
	static const int MAX_POLY_VERTS = 4;

	struct Point {
		float x, y, z;
		float nx, ny, nz;
		float r, g, b, a;
		float u, v;
	};

	struct Poly {
		int nvtx;
		int ipnt[MAX_POLY_VERTS];
	};

protected:
	std::vector<Point> mPnts;
	std::vector<Poly> mPols;

	bool load_pnts(const std::string& pntsPath);
	bool load_pols(const std::string& polsPath);
public:
	TDGeometry();

	std::uint32_t get_pnt_num() const { return (uint32_t)mPnts.size(); }
	std::uint32_t get_poly_num() const { return (uint32_t)mPols.size(); }

	Point get_pnt(uint32_t idx) const {
		Point pnt = {};
		if (idx < get_pnt_num()) {
			pnt = mPnts[idx];
		}
		return pnt;
	}

	Poly get_poly(uint32_t idx) const {
		Poly poly = {};
		if (idx < get_poly_num()) {
			poly = mPols[idx];
		}
		return poly;
	}

	const std::vector<Point>& pnts() const { return mPnts; }
	const std::vector<Poly>& pols() const { return mPols; }

	bool load(const std::string& folder);
	bool load(const std::string& pntsPath, const std::string& polsPath);
	void unload();
	bool dump_geo(std::ostream& os) const;

	friend std::ostream& operator << (std::ostream& os, TDGeometry& geo) {
		geo.dump_geo(os);
		return os;
	}
};
