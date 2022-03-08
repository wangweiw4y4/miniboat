#include "VirtualForceField.h"

using namespace std;

polygon_t::polygon_t(const vector<point_t>& vertexes, forceparameters_t para1) : para(para1)
{
	//check for error
	if (vertexes.size() < 3)
		throw("A polygon_t should have at least three vertexes!");

	//reserve capacity
	segs.reserve(vertexes.size());

	//process point_ts
	vector<point_t> vertexes1 = vertexes;
	for (point_t& i : vertexes1)
		i = ProcessVertex(i);

	//add line_t segments
	segs.push_back(line_t{ vertexes1.back(), vertexes1.front() });
	for (auto i = vertexes1.begin(); i != vertexes1.end() - 1; i++)
		segs.push_back(line_t{*i, * (i + 1)});
}

void polygon_t::AddPolygon(const vector<point_t>& vertexes)
{
	//do nothing if there's less than 3 vertexes
	if (vertexes.size() < 3)
		return;

	//reserve capacity
	segs.reserve(segs.size() + vertexes.size());

	//process point_ts
	vector<point_t> vertexes1 = vertexes;
	for (point_t& i : vertexes1)
		i = ProcessVertex(i);

	//add line_t segments
	segs.push_back(line_t{ vertexes1.back(), vertexes1.front() });
	for (auto i = vertexes1.begin(); i != vertexes1.end() - 1; i++)
		segs.push_back(line_t{*i, * (i + 1)});
}

short polygon_t::InsideQ(point_t& pt)
{
	unsigned int count = 0;
	point_t tp = Processpoint_t(pt);

	//calculate inside or outside
	for (auto i : segs)
	{
		if ((i.pt1.x != i.pt2.x) && ((i.pt1.x<tp.x && i.pt2.x>tp.x) || (i.pt1.x > tp.x&& i.pt2.x < tp.x)) && (i.pt1.y * (i.pt2.x - tp.x) + i.pt2.y * (tp.x - i.pt1.x)) / (i.pt2.x - i.pt1.x) > tp.y)
			count++;
	}

	//return
	if (count % 2) return -1;
    else return 1;
}

point_t polygon_t::Nearest(point_t& pt)
{
	//initialiation
	point_t pout = SegNearest(segs[0], pt), ptemp = { 0,0 };
	double shortest = (!pout), stemp = 0;

	//calculate all segments
	for (auto i : segs)
	{
		stemp = !(ptemp = SegNearest(i, pt));
		if (stemp < shortest)
		{
			shortest = stemp;
			pout = ptemp;
		}
	}

	return pout;
}

point_t polygon_t::RegionForce(point_t& pt)

{
	short s = InsideQ(pt);
	point_t dir = Nearest(pt);
	double ldir = (~dir); //what is ldir? what does it represent? ldir is the length of dir vector. This code is correct.

	return dir * (para.regionf1 * (1.0 + tanh(ldir * s / para.regionf2)) * s / (ldir + 0.0001));
}

point_t polygon_t::ProcessVertex(point_t& pt)
{
	return point_t{(round(pt.x / PolyResolution - 0.5) + 0.5)* PolyResolution, (round(pt.y / PolyResolution - 0.5) + 0.5)* PolyResolution};
}

point_t polygon_t::Processpoint_t(point_t& pt)
{
	return point_t{round(pt.x / PolyResolution)* PolyResolution, round(pt.y / PolyResolution)* PolyResolution};
}

point_t polygon_t::SegNearest(line_t& l, point_t& pt) 
{ 
    double dot1 = 0, dot2 = 0; 
    if ((dot1 = ((pt - l.pt1) & (l.pt2 - l.pt1))) <= 0) return l.pt1 - pt; 
    else if ((dot2 = ((pt - l.pt2) & (l.pt1 - l.pt2))) <= 0) return l.pt2 - pt; 
    else return (l.pt2 * dot1 + l.pt1 * dot2) / (dot1 + dot2) - pt;
}

double RepulseForce(const double x1, const double x2, forceparameters_t para)
{
	return para.repulsef1 * tanh(para.repulsef2 * (((x1 > x2) ? x1 : x2) - para.repulsef3))* ((x1 - x2) / (x1 + x2));
}
