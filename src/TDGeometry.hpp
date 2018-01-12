#include <ostream>
#include <vector>

class cTDGeometry {
public:
	static const int MAX_POLY_VERTS = 4;

	struct sPoint {
		float x, y, z;
		float nx, ny, nz;
		float r, g, b, a;
		float u, v;
	};

	struct sPoly {
		int nvtx;
		int ipnt[MAX_POLY_VERTS];
	};

protected:
	std::vector<sPoint> mPnts;
	std::vector<sPoly> mPols;

	bool load_pnts(const std::string& pntsPath);
	bool load_pols(const std::string& polsPath);
public:
	cTDGeometry();

	std::uint32_t get_pnt_num() const { return (uint32_t)mPnts.size(); }
	std::uint32_t get_poly_num() const { return (uint32_t)mPols.size(); }

	sPoint get_pnt(uint32_t idx) const {
		sPoint pnt = {};
		if (idx < get_pnt_num()) {
			pnt = mPnts[idx];
		}
		return pnt;
	}

	sPoly get_poly(uint32_t idx) const {
		sPoly poly = {};
		if (idx < get_poly_num()) {
			poly = mPols[idx];
		}
		return poly;
	}

	const std::vector<sPoint>& pnts() const { return mPnts; }
	const std::vector<sPoly>& pols() const { return mPols; }

	bool load(const std::string& folder);
	bool load(const std::string& pntsPath, const std::string& polsPath);
	bool dump_geo(std::ostream& os) const;

	friend std::ostream& operator << (std::ostream& os, cTDGeometry& geo) {
		geo.dump_geo(os);
		return os;
	}
};
