#ifndef __VIRdoubleUALFORCEFIELD_H__
#define __VIRdoubleUALFORCEFIELD_H__
#pragma once

#include <stdio.h>
#include <math.h>
#include <vector>

//regionf1 -- scaling of regional force
//regionf2 -- how wide is the transition zone
//repulsef1 -- scaling of repulsive force
//repulsef2 -- at how much force would repulsive force saturate
//repulsef3 -- defines the light intenity at the stable distance
struct forceparameters_t
{
	double regionf1;
	double regionf2;

	double repulsef1;
	double repulsef2;
	double repulsef3;
};

//point_t class
class point_t
{
public:
	double x;
	double y;

	point_t operator + (point_t pt2) { return point_t{ x + pt2.x,y + pt2.y }; }
	point_t operator - (point_t pt2) { return point_t{ x - pt2.x,y - pt2.y }; }
	point_t operator - () { return point_t{ -x,-y }; }
	point_t operator * (double val) { return point_t{ val * x,val * y }; }
	point_t operator / (double val) { return point_t{ x / val,y / val }; }
	point_t operator += (point_t pt2) { x += pt2.x; y += pt2.y; return *this; }
	point_t operator -= (point_t pt2) { x -= pt2.x; y -= pt2.y; return *this; }
	point_t operator *= (double val) { x *= val; y *= val; return *this; }
	point_t operator /= (double val) { x /= val; y /= val; return *this; }

	//dot product
	double operator & (point_t pt2) { return x * pt2.x + y * pt2.y; }
	//cross product
	double operator ^ (point_t pt2) { return x * pt2.y - y * pt2.x; }
	//norm
	double operator ~ () { return sqrt(x * x + y * y); }
	//norm^2
	double operator ! () { return x * x + y * y; }
};

//line_t segment class
struct line_t
{
	point_t pt1;
	point_t pt2;
};


//point_t resolution used in polygon_t detection and calculation
//vertexes are round to (N+0.5)*PolyResolution where N is an integer
//input point_ts are round to N*PolyResolution
constexpr double PolyResolution = 0.00001;

//polygon_t class
class polygon_t
{
public:
	//all line_t segments
	std::vector<line_t> segs;
	//force parameters
	forceparameters_t para;

	//initialization function
	polygon_t(const std::vector<line_t>& segs1, forceparameters_t para1) : segs(segs1), para(para1) {}
	//recommend to use this function for initialization
	polygon_t(const std::vector<point_t>& vertexes, forceparameters_t para1);

	//add another polygon_t
	//note that the addition is XOR instead of AND
	//i.e. overlapping region will be considered as outside the region
	void AddPolygon(const std::vector<point_t>& vertexes);


	//determine whether the point_t is inside or outside the polygon_t
	//this function may return 1 or -1
	//-1 indicates that the point_t is inside the polygon_t
	//+1 indicates that the point_t is outside the polygon_t
	short InsideQ(point_t& pt);

	//determine the relative position of the nearest point_t on the edge of the polygon_t
	point_t Nearest(point_t& pt);

	//calculate the force exerted by the region
	point_t RegionForce(point_t& pt);

private:
	//vertexes are round to (N+0.5)*PolyResolution where N is an integer
	point_t ProcessVertex(point_t& pt);
	//input point_ts are round to N*PolyResolution
	point_t Processpoint_t(point_t& pt);

	//determine the relative position of the nearest point_t on a line_t segment
	point_t SegNearest(line_t& l, point_t& pt);
};

//repulsive force is determined by comparing light intensity received in the opposite directions
double RepulseForce(const double x1, const double x2, forceparameters_t para);

#endif
