#include "Region.h"

int Region::regionCount = 0;
//float Region::pointSelectionThreshold = 81.f;

//bool Region::editingRegions = false;
//Region::REGION_EDIT_ACTION Region::regionEditAction = REGION_EDIT_ACTION::NONE;  //what a beautiful line of code
//cv::Point2f Region::movingRefPt(0,0);

//cv::Size Region::imgSize(99999, 99999);
//bool Region::allowMultipleRegions = false;
//bool Region::allowNonrectangularity = false;

//Region* Region::pSelectedRegion = nullptr;

Region::~Region(void)
{
    std::cout << "Deleting Region " << regionLabel << std::endl;
	Delete();
}


void Region::Delete()
{	
	--Region::regionCount;
}


Region::Region(void) :
	regionNo(Region::regionCount),
	regionType(0)
{
	Initialize();
	regionLabel = std::to_string(regionNo);
}


Region::Region(const cv::Rect& inputRect, const std::string _regionLabel, const uchar _regionType) :
	regionNo(Region::regionCount),
	regionType(_regionType)
{
	Initialize();
	Set_Region_To_Rect(inputRect);
	bRect = inputRect;
	regionLabel = (_regionLabel=="") ? std::to_string(regionNo) : _regionLabel;
}


Region::Region(const std::vector<cv::Point2f>& inputPolygon, const std::string _regionLabel, const uchar _regionType) :
	regionNo(Region::regionCount),
	regionType(_regionType)
{
	Initialize();
	polyBound = inputPolygon;
	Update_Bounding_Rect();
	regionLabel = (_regionLabel=="") ? std::to_string(regionNo) : _regionLabel;
}


void Region::Initialize(void)
{
	isSelected = false;
	pSelectedPt = nullptr;
	allowNonrectangularity = false;
	imgSize = cv::Size(99999, 99999);
	++Region::regionCount;
}


Region::Region(const Region& other) :
	regionNo(Region::regionCount)
{
	this->Initialize();
	*this = other;
}


Region& Region::operator=(const Region& other)
{
	this->bRect = other.bRect;
	this->polyBound = other.polyBound;
	this->regionNo = other.regionNo;
	this->regionLabel = other.regionLabel;
	this->regionType = other.regionType;
	return *this;
}


void Region::Set_Region_To_Rect(const cv::Rect& _bRect)
{
	polyBound.resize(4);
	polyBound[0] = cv::Point2f(_bRect.x,  _bRect.y);
	polyBound[1] = cv::Point2f(_bRect.x + _bRect.width, _bRect.y);
	polyBound[2] = cv::Point2f(_bRect.x + _bRect.width, _bRect.y + _bRect.height);
	polyBound[3] = cv::Point2f(_bRect.x,  _bRect.y + _bRect.height);
	bRect = _bRect;
}


void Region::Update_Bounding_Rect(void)
{
	if (!allowNonrectangularity) bRect = cv::Rect(polyBound[0], polyBound[2]);
	else  bRect = cv::boundingRect(polyBound);
}

//if you simply want a binary, yes/no, the point is in/out of region. call this.
bool Region::Contains(const cv::Point& pt)
{
	return (cv::pointPolygonTest(polyBound, pt, false) > -0.1 );
}


void Region::Draw_Region(cv::Mat& image, bool showHandles, bool showOutline, bool showRegionLabel, float scl, double fontSize)
{ 
	cv::Scalar drawColor = isSelected ? cv::Scalar(0,255,0) : cv::Scalar(255,255,255);
	
	int leftTextMargin = 2;
	int lineSpaceY = 24;
	int topTextMargin = (showRegionLabel ? 12*fontSize : 12*fontSize-lineSpaceY);
	int fontStyle = cv::FONT_HERSHEY_PLAIN;
    size_t ptCnt = polyBound.size();

	if (showOutline)
	{
		for (size_t ptNo=0; ptNo<ptCnt; ptNo++)
		{
            //cv::Scalar lineColor = (ptNo==selectedEdgeNo) ? drawColor : cv::Scalar(0,0,0);
            cv::line(image, polyBound[ptNo]*scl, polyBound[(ptNo+1)%ptCnt]*scl, drawColor, 1, cv::LineTypes::LINE_AA);
		}
	}

	if (showRegionLabel)
		cv::putText(image, regionLabel, scl*polyBound[0]+cv::Point2f(leftTextMargin,topTextMargin), fontStyle, fontSize, drawColor, 1, cv::LINE_AA);


	if (isSelected || showHandles) 
	{
		for (cv::Point2f& pt : polyBound)
		{
            //cv::Scalar circleColor = (&pt==pSelectedPt) ? drawColor : cv::Scalar(0,0,0);
            cv::circle(image, scl*(pt), 2, drawColor, 2);					//highlight the selected region
		}
		cv::putText(image, "[" + std::to_string(bRect.width) + "x" + std::to_string(bRect.height) + "]", scl*polyBound.back()+cv::Point2f(leftTextMargin,-topTextMargin-lineSpaceY), fontStyle, fontSize, drawColor, 1, cv::LINE_AA);
	}  
}


bool Region::Read_From_Text(std::string& lnTxt)
{
	std::string _regionLabel="";
	size_t delimPos=0, lastPos=0, delimPos2;
	//int _regionType=0;

	delimPos = lnTxt.find("~", lastPos);
	if (delimPos != std::string::npos)
	{
		regionLabel = lnTxt.substr(lastPos,delimPos);
		lastPos = delimPos+1;
	}

		
	//delimPos = lnTxt.find("~", lastPos);
	//if (delimPos != std::string::npos)
	//{
	//	_regionType = std::stoi(lnTxt.substr(lastPos,delimPos));
	//	lastPos = delimPos+1;
	//}

	lastPos = lnTxt.rfind("~");
	if (lastPos == std::string::npos)  lastPos = 0;

	float x, y;
	while ( true )
	{
		delimPos2 = lnTxt.find("|", lastPos);
		if (delimPos2 == std::string::npos)
			break;

		delimPos = lnTxt.find(" ", lastPos); 
		if (delimPos == std::string::npos)           //this should never happen if the file wasn't altered, but in case there's some text without a "|", it's some sort of error, ignore
			delimPos = lnTxt.length(); 

		x = std::stoi(lnTxt.substr(lastPos,delimPos2));
		y = std::stoi(lnTxt.substr(delimPos2+1,delimPos));  
		polyBound.push_back(cv::Point2f(x,y));
		lastPos = delimPos+1;
	}

	return (polyBound.size() > 2);	//return true if the region has area (3+ points)
}


void Region::Write_To_Text(std::ostream& xout)
{
	xout << regionLabel << "~";
	xout << std::to_string(regionType) << "~";
	for (const cv::Point& pt : polyBound)
		xout << pt.x << "|" << pt.y << " ";

	xout << std::endl;
}


std::string Region::Get_Type_Name()
{
	//std::string typeName;
	//Get_Region_Type_Name(regionType, typeName);
	//return typeName;
	return "";
}

//this version first checks if the point is near or inside the region and if so checks if the point is close to the edge. Returns via params whether point is close to an edge and 'which edge' that is (order within polyBound) 
bool Region::Contains_And_Near_Edge_Check(const cv::Point& mousePt, bool& nearEdge, int& nearestEdgeStartPt)
{
	nearEdge = false;
	double thresh = 4;
	double result = cv::pointPolygonTest(polyBound, mousePt, true);
	if (abs(result) < thresh)
	{ 
		nearEdge = true;
		nearestEdgeStartPt = this->Nearest_Edge_Start_Point(mousePt);
		return true;
	}
	else if (result > 0) {return true;}
	return false;
}

//determine closest edge on region boundary to point clicked. Used to add an editable point to the boundary from the UI (clicking on the image)
//basically project the vector from edge endpoint to point clicked and, provided the projection falls on the edge segmen (edge has finite length, line does not), select the edge with the shortest perpendicular distance
int Region::Nearest_Edge_Start_Point(const cv::Point2f& mousePt)
{
	cv::Point2f ptProjVec, ptNormVec;
	float distSqr, bestDist=9999;
	int bestPtNo=0;
    for (size_t ptNo=0; ptNo<polyBound.size(); ptNo++)
	{
		if (!Decompose_Edge_To_Point_Vector(polyBound[(ptNo+1)%polyBound.size()], polyBound[ptNo], mousePt, ptNormVec, ptNormVec))
			continue;

		distSqr = ptNormVec.dot(ptNormVec);									//shortest distance from edge to mouse pt
		if (distSqr < bestDist)  { bestDist = distSqr; bestPtNo = ptNo; }
	}

	return bestPtNo;
}


void Region::Move_Translate(const cv::Point2f& distMoved)
{
	cv::Point2f minLegalMove = distMoved;
	for (cv::Point2f& pt : polyBound)
	{
		cv::Point2f ptCopy = pt+minLegalMove;
		if (!Bound_Point(ptCopy, imgSize))
			minLegalMove = ptCopy - pt;
	}
	for (cv::Point2f& pt : polyBound)  pt += minLegalMove;
	Update_Bounding_Rect();
}

//moves the 2 points forming the selected edge so the edge passes through mousePt. Does not require a rectangular selection
void Region::Move_Edge(const cv::Point2f& mousePt, int edgeNo)
{
	if (edgeNo == -1)  edgeNo = selectedEdgeNo;

	cv::Point2f ptProjVec, ptNormVec;
	cv::Point2f* pEdgeStart = &polyBound[edgeNo];
	cv::Point2f* pEdgeEnd = &polyBound[(edgeNo+1)%polyBound.size()];
	Decompose_Edge_To_Point_Vector(*pEdgeStart, *pEdgeEnd, mousePt, ptNormVec, ptNormVec);
	
	*pEdgeStart += ptNormVec;
	*pEdgeEnd += ptNormVec;

	Bound_Point(*pEdgeStart, imgSize);
	Bound_Point(*pEdgeEnd, imgSize);
	Update_Bounding_Rect();
}


void Region::Move_Point(const cv::Point2f& newLoc, cv::Point2f* pPolyBoundPt)
{
	if (pPolyBoundPt==nullptr)  pPolyBoundPt = pSelectedPt;

	//if allowing nonrectangularity, we simply move the point (although user could do something silly like cross other edges to create overlap and such - no protection)
	*pPolyBoundPt = newLoc;
	Bound_Point(*pPolyBoundPt, imgSize);
	
	//To maintain rectangle we need to flip the rectangle around the stationary corner if user contorts the rectangle...
	if (!allowNonrectangularity)
	{
		//I'm using the point number to figure out where in the recangle we are (assuming first in vector is upper left). Could probably do some pointer arithmetic instead, but enh.
		size_t ptNo = 0;
		while (true)
		{
			if (pPolyBoundPt == &polyBound[ptNo])
			{
				//cv::Point2f vertex = polyBound[ptNo]; //I think I need a copy of this? I forget what I was thinking when I wrote it...
				if (ptNo%2==0)	{ polyBound[(ptNo+3)%4].x = newLoc.x;  polyBound[(ptNo+1)%4].y = newLoc.y; }
				else			{ polyBound[(ptNo+3)%4].y = newLoc.y;  polyBound[(ptNo+1)%4].x = newLoc.x; }
				break;
			}
			++ptNo;
		}
	}

	Update_Bounding_Rect();
}


void Region::Add_Point(const cv::Point2f& newPt, int edgeNo)
{
	polyBound.insert(polyBound.begin()+(edgeNo+1), newPt);
	Update_Bounding_Rect();
}

//Maybe make these all non-static at some point if it meshes with the resizing business.
void Region::Set_Img_Size(cv::Size _imgSize)
{
	imgSize = _imgSize;
}


void Region::Set_Allow_Nonrectangularity(bool allow)
{
	allowNonrectangularity = allow;
}
