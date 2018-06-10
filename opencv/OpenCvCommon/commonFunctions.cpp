#include "commonFunctions.h"

bool Bound_Point(cv::Point2f& pt, const cv::Size& imgSize)
{
    bool wasChanged = false;

    if (pt.x < 0)						    { pt.x = 0;  wasChanged=true; }
    else if (pt.x >= imgSize.width)   { pt.x = imgSize.width-1;  wasChanged=true; }

    if (pt.y < 0)						    { pt.y = 0;  wasChanged=true; }
    else if (pt.y >= imgSize.height)  { pt.y = imgSize.height-1;  wasChanged=true; }

    return !wasChanged;
}

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

bool Decompose_Edge_To_Point_Vector(const cv::Point2f& edgeStartPt, const cv::Point2f& edgeEndPt, const cv::Point2f& pt, cv::Point2f& ptNormVec, cv::Point2f& ptProjVec )
{
    cv::Point2f edgeVec = edgeEndPt - edgeStartPt;					//edge vector
    float edgeLength = sqrt(edgeVec.dot(edgeVec));					//length of edge vector
    cv::Point2f ptVec = pt - edgeStartPt;							//vector from edge start to mouse point
    float ptProjLength = ptVec.dot(edgeVec)/edgeLength;				//length of projection of ptVec onto edge
    ptProjVec = edgeVec*ptProjLength/edgeLength;
    ptNormVec = ptVec - ptProjLength*edgeVec/edgeLength;			//the component of ptVec normal to the edge (distance vector from edge to point)
    return  ( ptProjLength >= 0 && ptProjLength <= edgeLength );	//return value: true if mousePt is 'within' the edge as opposed to outside. i.e., the point projects onto the edge line segment
}

cv::Point2f Scale_To_Image(const cv::Point2f& rawPt, const cv::Size& imgSize, float xMax, float xMin, float yMax, float yMin)
{
    return cv::Point2f( imgSize.width*(rawPt.x-xMin)/(xMax-xMin), imgSize.height*(1-(rawPt.y-yMin)/(yMax-yMin)) );
}

void Get_Timestamp(std::string& timestampStr, const char* formatStr)
{
    auto t = time(nullptr);
    auto tm = *localtime(&t);
    std::stringstream timestamp;
    timestamp << std::put_time(&tm, formatStr);
    timestampStr = timestamp.str();
}

cv::Mat imagesc(const std::string& title, const cv::Mat& cv32fImage, bool showImage, bool showLegend)
{
    cv::Mat u8image, cMappedImage;
    double minVal, maxVal;
    cv::minMaxIdx(cv32fImage, &minVal, &maxVal, nullptr, nullptr);

    cv32fImage.convertTo(u8image, CV_8U, 255.0/(maxVal-minVal), -minVal*255.0/(maxVal-minVal));
    cv::applyColorMap(u8image, cMappedImage, cv::ColormapTypes::COLORMAP_JET);

    if (showImage)
    {
        cv::namedWindow(title, cv::WINDOW_NORMAL);
        cv::imshow(title, cMappedImage);
    }

    if (!showLegend)
        return cMappedImage;

    int width=64, height=256;
    cv::Mat legend(height,width,CV_8UC3);
    cv::Mat legendU8(height,width,CV_8UC1);

    for (uchar sect=0; sect<8; sect++)
        cv::rectangle(legendU8, cv::Rect(0,sect*(height/8),width,height/8), cv::Scalar((8-sect)*32), CV_FILLED);

    cv::applyColorMap(legendU8, legend,cv::ColormapTypes::COLORMAP_JET);

    char cBuff[8];
    for (uchar sect=0; sect<8; sect++)
    {
        sprintf_s(cBuff, "%.1f", minVal+(7.5-sect)/8*(maxVal-minVal));
        cv::putText(legend,cBuff, cv::Point(2,(0.5+sect)*height/8), cv::HersheyFonts::FONT_HERSHEY_COMPLEX_SMALL, 1, cv::Scalar(255,255,255), 1, CV_AA);
    }
    cv::namedWindow("Legend", cv::WINDOW_AUTOSIZE);
    cv::imshow("Legend", legend);

    return cMappedImage;
}


//converts an HSV tuple to BGR Scalar. Based on some public algorithm
//h = hue			Range = 0-360	  0 is red, 180 is teal and 360 is red again
//s = saturation	Range = 0-255	  0 is gray/black/white and 255 is colorful
//v = value			Range = 0-255	  0 is black and 255 is bright
cv::Scalar HSV2BGR(int h, int s, int v)
{
    if ( s == 0 || v == 0 )
        return cv::Scalar(v, v, v);

    int r, g, b, sect;
    float fh, frac, p, q, t;

    h %= 360;
    fh = static_cast<float>(h)/60.f;
    sect = std::floor( fh );	// sect is like the section of the color wheel
    frac = fh - sect;			// fractional part of h
    p = v * ( 255 - s )/255;
    q = v * ( 255 - s * frac )/255;
    t = v * ( 255 - s * ( 1 - frac ) )/255;
    switch( sect ) {
        case 0:		r = v;	g = t;	b = p;	break;
        case 1:		r = q;	g = v;	b = p;	break;
        case 2:		r = p;	g = v;	b = t;	break;
        case 3:		r = p;	g = q;	b = v;	break;
        case 4:		r = t;	g = p;	b = v;	break;
        default:	r = v;	g = p;	b = q;	break;
    }
    return cv::Scalar(b,g,r);
}
