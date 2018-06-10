#pragma once 
#include "opencv2/core.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include <iomanip>
#include <ctime>
#include <string>

template <class _pt>
float Sqrd_Dist(const _pt& pt1, const _pt& pt2)
{
	float diff = pt1.x - pt2.x;
	float distSqr = diff*diff;
	diff = pt1.y - pt2.y;
	distSqr += diff*diff;
	return distSqr;
}

template <class T_pt>
float Dist(const T_pt& pt1, const T_pt& pt2)
{
	return sqrt(Sqrd_Dist(pt1,pt2));
}

bool Bound_Point(cv::Point2f& pt, const cv::Size& imgSize);

//template <class T_pt>
//T_pt Bound_Point2(T_pt& pt, const cv::Size& imgSize)
//{
//	T_pt boundedPt = pt;
//
//	if (boundedPt.x < 0)						    { boundedPt.x = 0; }
//	else if (boundedPt.x >= imgSize.width)  { boundedPt.x = imgSize.width-1; }
//
//	if (boundedPt.y < 0)						    { boundedPt.y = 0; }
//	else if (boundedPt.y >= imgSize.height) { boundedPt.y = imgSize.height-1; }
//	
//	return boundedPt;
//}

bool Decompose_Edge_To_Point_Vector(const cv::Point2f& edgeStartPt, const cv::Point2f& edgeEndPt, const cv::Point2f& pt, cv::Point2f& ptNormVec, cv::Point2f& ptProjVec );

cv::Point2f Scale_To_Image(const cv::Point2f& rawPt, const cv::Size& imgSize, float xMax, float xMin, float yMax, float yMin);

void Get_Timestamp(std::string& timestampStr, const char* formatStr="%Y-%m-%d_%H-%M-%S");

cv::Mat imagesc(const std::string& title, const cv::Mat& cv32fImage, bool showImage=false, bool showLegend=false);

cv::Scalar HSV2BGR(int h, int s, int v);
