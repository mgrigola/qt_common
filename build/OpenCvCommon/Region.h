#pragma once

#include "opencv2/core.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/highgui.hpp"
#include <vector>
#include <math.h>
#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include "commonFunctions.h"


class Region
{
private:
	static int regionCount;
	//static bool editingRegions;  //, verbose, allowMultipleRegions, allowNonrectangularity;
	//static cv::Point2f movingRefPt;

public:
	cv::Size imgSize;
	bool allowNonrectangularity;
	bool isSelected;
    size_t selectedEdgeNo;
	cv::Point2f* pSelectedPt;

	void Initialize(void);

public:
	//enum REGION_TYPE : uchar {
	//	GENERIC,					// The default region type unless otherwise specified. No special handling
	//	OPEN,						// The region is open space where cells are unobstructed. We might get baseline size/speed/shape/etc. here. Potentially pais with a  region of another type for comparison
	//	CHANNEL,					// The region covers a slit/channel where cells are deformed and may get stopped/stuck
	//	INLET,						// The region is the area where the flow constricts before a channel. Should pair with a channel. We might look for cells that don't pass cleanly and flag them as unclean in the paired channel
	//	OBSTRUCTIONS,				// The region covers an area of obstructions/pillars where the cell may bounce around. We'll look for segments where the cell travels in a straight path for measurements
	//	WHOLE,						// A region optimized to cover the entire field of view? It's there if needed....
	//	BASELINE,					// An open region used for establishing baseline numbers for cells. If, the cell's area and initial speed are set to the max/average in this region
	//	REGION_TYPE_COUNT			// Not a region type, just the marker for the end of the list and count fo items in it
	//};
	int regionNo;
	std::string regionLabel;
	uchar regionType;

	cv::Rect bRect;
	std::vector<cv::Point2f> polyBound;

    virtual ~Region(void);  //if class is intended to be derived from. destructor must be virtual! (I think derived class' destructor won't getcalled if not, even though they appear to have different names).
	Region(void);
	Region(const cv::Rect& inputRect, const std::string _regionLabel="", const uchar _regionType=0);
	Region(const std::vector<cv::Point2f>& inputPolygon, const std::string _regionLabel="", const uchar _regionType=0);
	Region(const Region&);
	Region& operator=(const Region& other);
	
	void Set_Region_To_Rect(const cv::Rect& inputRect);
	void Update_Bounding_Rect(void);
	void Move_Translate(const cv::Point2f& distMoved);
	void Move_Edge(const cv::Point2f& mousePt, int edgeNo=-1);
	void Move_Point(const cv::Point2f& mousePt, cv::Point2f* pVertex=nullptr);
	void Add_Point(const cv::Point2f& newPt, int edgeNo);

	void Delete(void);

	int Nearest_Edge_Start_Point(const cv::Point2f& mousePt);
	bool Contains(const cv::Point& pt);
	bool Contains_And_Near_Edge_Check(const cv::Point& pt, bool& nearEdge, int& edgeInsertionPos);

	virtual void Draw_Region(cv::Mat& image, bool showHandles=false, bool showOutline=true, bool showRegionLabel=false, float scl=1.f, double fontSize=0.75);
	virtual void Write_To_Text(std::ostream& xout);
	virtual bool Read_From_Text(std::string& lnTxt);
	virtual std::string Get_Type_Name();

	void Set_Img_Size(cv::Size _imgSize);
	void Set_Allow_Nonrectangularity(bool allow);
};
