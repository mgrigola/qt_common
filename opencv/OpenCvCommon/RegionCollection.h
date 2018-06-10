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
#include "Region.h"

enum REGION_EDIT_ACTION : uchar {
	NONE,					
	EDITING,					
	CREATING_NEW_REGION,
	RESIZING_REGION,			
	MOVING_REGION,				
	MOVING_POINT		
};

template <class T_Region>
class RegionCollection
{
private:
	//int regionCount;
	//bool editingRegions;  //, verbose, allowMultipleRegions, allowNonrectangularity;
	float pointSelectionThreshold;
	cv::Size imgSize;
	bool verbose, allowMultipleRegions, allowNonrectangularity;
	
	T_Region* pSelectedRegion;
	cv::Point2f movingRefPt;


	bool Callback_New_Region_Check(const cv::Point2f& mousePt, int event, int flags);
	bool Callback_Resize_Check(const cv::Point2f& mousePt, int event, int flags);
	bool Callback_Region_Move_Check(const cv::Point2f& mousePt, int event, int flags);
	bool Callback_Point_Move_Check(const cv::Point2f& mousePt, int event, int flags);
	bool Callback_New_Region_Definition(const cv::Point2f& mousePt, int event, int flags);
	bool Callback_Region_Selection(const cv::Point2f& mousePt, int event, int flags);
	bool Callback_Point_Selection(const cv::Point2f& mousePt, int event, int flags);
	//void Move_Selected_Edge(const cv::Point2f& mousePt);
	//int Nearest_Edge_Start_Point(const cv::Point2f& mousePt);
	
	

public:
	REGION_EDIT_ACTION regionEditAction;
	std::vector<T_Region*> regions;


    RegionCollection(bool _verbose = false);
	~RegionCollection(void);

	std::string Get_Regions_File_Name(const std::string& fullFileName);
	bool Read_Regions(const std::string& fullFileName);
	bool Write_Regions(const std::string& fullFileName);
	//bool Translate_Type(uchar type, std::string& outputStr);
    bool Callback(const cv::Point2f& mousePt, int event, int flags);
	void Draw_Regions(cv::Mat& image, bool showStats=false, bool showHandles=false, bool showOutline=true, bool showRegionLabel=false, float scl=1.f, double fontSize=0.75);

	void Add_Region(const T_Region& regionToAdd);
	void Clear(void);
	void Delete_Selected(bool deleteLastIfNoneSelected=true);

	void Set_Image_Size(cv::Size _imgSize);
	void Set_Allow_Multiple_Regions(bool allow);
	void Set_Allow_Nonrectangularity(bool allow);

    cv::Size Get_Image_Size(void);
    bool Get_Allow_Multiple_Regions(void);
    bool Get_Allow_Nonrectangularity(void);

	inline int size(void) { return regions.size(); }
	inline T_Region* operator[](size_t regionNo)  { return regions[regionNo]; }
};

template <class T_Region>
RegionCollection<T_Region>::RegionCollection(bool _verbose) :
	imgSize(99999,99999),
	allowMultipleRegions(true),
	allowNonrectangularity(false),
    pointSelectionThreshold(36),
    verbose(_verbose)
{
}

template <class T_Region>
RegionCollection<T_Region>::~RegionCollection(void)
{
}

//void RegionCollection::Delete_Selected(bool deleteLastIfNoneSelected)
//{
//	bool deletedSomething = false;
//	for (size_t regNo=0; regNo<existingRegions.size(); regNo++)
//	{
//		if (existingRegions[regNo].isSelected)
//		{
//			existingRegions.erase(existingRegions.begin()+regNo);
//			deletedSomething = true;
//		}
//	}
//
//	if (!deletedSomething && existingRegions.size()>0)
//	{
//		(existingRegions.back()).Delete();
//		existingRegions.pop_back();
//	}
//}

template <class T_Region>
void RegionCollection<T_Region>::Add_Region(const T_Region& regionToAdd)
{
	T_Region* pNewRegion = new T_Region(regionToAdd);   //copy the passed-in region (this is like vector.push_back)
	regions.push_back(pNewRegion);
}

template <class T_Region>
void RegionCollection<T_Region>::Delete_Selected(bool deleteLastIfNoneSelected)
{
	bool deletedSomething = false;
	int regNo = regions.size();
	while(--regNo>=0)
	{
		if (regions[regNo]->isSelected)
		{
			delete regions[regNo];
			regions.erase(regions.begin()+regNo);
			deletedSomething = true;
		}
	}

	if (!deletedSomething && regions.size()>0)
	{
		(regions.back())->Delete();
		regions.pop_back();
	}

	pSelectedRegion = nullptr;
}

template <class T_Region>
std::string RegionCollection<T_Region>::Get_Regions_File_Name(const std::string& fullFileName)
{
    //return fullFileName.substr(0, fullFileName.rfind(".")) + ".regions";
    return fullFileName + ".regions";
}

//read the file of saved regions created by Write_Regions. Foramt is like: one region per row, each region looks like:  name~type~pt1.x|pt1.y pt2.x|pt2.y ...
template <class T_Region>
bool RegionCollection<T_Region>::Read_Regions(const std::string& fullFileName)
{
	std::ifstream inputFile( Get_Regions_File_Name(fullFileName));
	if (!inputFile.is_open())
		return false;
	
	std::string lnTxt;
	while (std::getline(inputFile, lnTxt, '\n'))
	{
		T_Region* pNewRegion = new T_Region;
		if (pNewRegion->Read_From_Text(lnTxt))
		{
			pNewRegion->allowNonrectangularity = allowNonrectangularity;    //should use functions
			pNewRegion->imgSize = imgSize;
			regions.push_back(pNewRegion);
		}
		else  { delete pNewRegion; }
	}

	inputFile.close();
	return (regions.size() > 0);
}

//save the regions to a text file (Name_Of_Input_File.regions). Foramt is like: one region per row, each region looks like:  pt1.x|pt1.y pt2.x|pt2.y ...
template <class T_Region>
bool RegionCollection<T_Region>::Write_Regions(const std::string& fullFileName)
{
	if (regions.size() < 1)
		return false;

	std::string saveFileName = Get_Regions_File_Name(fullFileName);  
	std::ofstream outputFile(saveFileName, std::fstream::trunc);   //trunc means delete the file if it exists already
	for (T_Region* pRegion : regions)
		pRegion->Write_To_Text(outputFile);

	outputFile.close();
	return true;
}

template <class T_Region>
void RegionCollection<T_Region>::Draw_Regions(cv::Mat& image, bool showStats, bool showHandles, bool showOutline, bool showRegionLabel, float scl, double fontSize)
{
	for (T_Region* pRegion : regions)  pRegion->Draw_Region(image, showHandles, showOutline, showRegionLabel, scl, fontSize);
}

//This mess is the handling for interacting with the regions (adding/moving/reshaping regions, adding/moving points, etc.). Don't worry how it works. It's good enough to get the job done for now.
//for the params, follow one of the symbols CV_EVENT_* they actually don't have like a type, they're just unlabelled enums floating in highgui_c.h ick...
//this function returns true if any of the regions have changed at all (and thus need to redraw all the regions and probably the base image, too, because it's not super efficient)
template <class T_Region>
bool RegionCollection<T_Region>::Callback(const cv::Point2f& mousePt, int event, int flags)
{
	//Check if we're in the middle of a procedure like move or resize. If so, do the work for that procedure and return. Otherwise continue on deciphering what the input means.

    if (verbose)
        std::cout << "(rcAct:" << regionEditAction << " Event:" << event << ")\t";

    if (regionEditAction == REGION_EDIT_ACTION::CREATING_NEW_REGION)  { return Callback_New_Region_Check(mousePt, event, flags); }
    if (regionEditAction == REGION_EDIT_ACTION::RESIZING_REGION)      { return Callback_Resize_Check(mousePt, event, flags); }
    if (regionEditAction == REGION_EDIT_ACTION::MOVING_REGION)        { return Callback_Region_Move_Check(mousePt, event, flags); }
    if (regionEditAction == REGION_EDIT_ACTION::MOVING_POINT)         { return Callback_Point_Move_Check(mousePt, event, flags); }
	
    //user is starting to define a new region. Region will be finalized upon releasing rbutton
	if (event == CV_EVENT_RBUTTONDOWN)
        return Callback_New_Region_Definition(mousePt, event, flags);


	//No region currently selected and user left-clicked somewhere on image
	if (event == CV_EVENT_LBUTTONDOWN )
	{
		//if no region selected, the only possibility is to select a region
		if (pSelectedRegion == nullptr)
            return Callback_Region_Selection(mousePt, event, flags);
	
		//there is currently a selected region from here on down
	
        //check if user clicked near a point in the currently selected region. If so, select that point. Only checks the 'current' selected region
        if (Callback_Point_Selection(mousePt, event, flags))
            return true;
		
        //gather more info - see if click was near or in selected region, near one of its edges, or near one of its points. Otherwise, see if click was in another region
		int edgeStartPtNo;
		bool clickedNearEdge;
		bool mousePtInSelectedRegion = pSelectedRegion->Contains_And_Near_Edge_Check(mousePt, clickedNearEdge, edgeStartPtNo);

        //click IS near 'current' selected region.
        if ( mousePtInSelectedRegion )
        {
            //if user clicks near an edge with ALT pressed, then insert a new point where the user clicked. The region will no longer be rectangular!
            if (pSelectedRegion->allowNonrectangularity && clickedNearEdge && (flags & CV_EVENT_FLAG_ALTKEY) )
            {
                pSelectedRegion->Add_Point(mousePt, edgeStartPtNo);   //edgeStartPtNo is the first point on the edge. +1 will insert after the first point (and before the edge end point)
                if (verbose)  { std::cout << "Added point: " << pSelectedRegion->polyBound[edgeStartPtNo+1] << std::endl; }
                return true;
            }

            //Initiate edge-drag resizing
            else if (clickedNearEdge)
            {
                regionEditAction = REGION_EDIT_ACTION::RESIZING_REGION;
                pSelectedRegion->selectedEdgeNo = edgeStartPtNo;
                if (verbose)  { std::cout << "Resizing region: " << pSelectedRegion->regionLabel << std::endl; }
                return false;
            }

            //Initiate move regions
            else if (regionEditAction != REGION_EDIT_ACTION::MOVING_REGION)
            {
                regionEditAction = REGION_EDIT_ACTION::MOVING_REGION;
                movingRefPt = mousePt;
                if (verbose)  { std::cout << "Moving region(s) " << std::endl; }
                return false;
            }
        }

        //click is NOT near 'current' selected region. However it may be near another. If holding ctrl either select multiple regions or do nothing. If no ctrl, deselect current region and possibly select a differnt one.
		else
            return Callback_Region_Selection(mousePt, event, flags);
	}
    //else user isn't in the middle of a process and didn't click with a relevant mouse button. User probably mousing over the image or whatever. Not our problem.
    return false;
}

//If user is currently defining a new region, update the new region or finalize it and end callback.
//returns true if region changed, false if unchanged
template <class T_Region>
bool RegionCollection<T_Region>::Callback_New_Region_Check(const cv::Point2f& mousePt, int event, int flags)
{
	//Region::pSelectedRegion->Set_Region_To_Rect( cv::Rect(pSelectedRegion->polyBound[0], mousePt) );
    if (event == CV_EVENT_RBUTTONUP)  { regionEditAction = EDITING; }  //doesn't require updating bounding box. It's already a box
    cv::Point2f boundedPt = mousePt;
	Bound_Point(boundedPt, imgSize);
    cv::Rect newRect(movingRefPt, boundedPt);
    if (pSelectedRegion->bRect == newRect)
        return false;

    pSelectedRegion->Set_Region_To_Rect( newRect );
    return true;
}

//resize/move an edge of a region
//If user is currently resizing a new region, update the region or if mouse-up then finalize new region and end callback.
//returns true if region changed, false if unchanged
template <class T_Region>
bool RegionCollection<T_Region>::Callback_Resize_Check(const cv::Point2f& mousePt, int event, int flags)
{
    pSelectedRegion->Move_Edge(mousePt, pSelectedRegion->selectedEdgeNo);   //ideally, this function should return bool: changed or not. Can check if value of ptNormVec == (0,0). But for now I'm just gonna say it always changes - probably the usual case. No harm, just less efficient, because we need to redraw the whole image
	if (event == CV_EVENT_LBUTTONUP)
	{
		regionEditAction = REGION_EDIT_ACTION::EDITING;
		if (verbose)  { std::cout << "Stopped resizing " << pSelectedRegion->regionLabel << ". New region size: " << pSelectedRegion->bRect << std::endl; }
	}
    return true;
}

//move/translate one/multiple regions
//If user is currently moving region(s), move all polybound points distance the mouse has changed since last frame or finalize positions if mouse-up
template <class T_Region>
bool RegionCollection<T_Region>::Callback_Region_Move_Check(const cv::Point2f& mousePt, int event, int flags)
{
    //finalize/stop editing on mouseup. Does not change any regions
    if (event == CV_EVENT_LBUTTONUP)
    {
        regionEditAction = REGION_EDIT_ACTION::EDITING;
        if (verbose)
        {
            std::cout << "Stopped moving region: " << pSelectedRegion->regionLabel <<  "   New points";
            for (cv::Point2f& pt : pSelectedRegion->polyBound)  { std::cout << "\t" << pt;}
            std::cout << std::endl;
        }
    }

    //if current point and reference point are the same, no change occurred, don't do anything
    if (mousePt == movingRefPt)
        return false;

	//calculate the moved distance based on the primary-selected region's reference point
    cv::Point2f distMoved = mousePt - movingRefPt;
	movingRefPt = mousePt;

	//potentially move multiple regions
	for (T_Region* pRegion : regions)
	{
        if (pRegion->isSelected)
            pRegion->Move_Translate(distMoved);
	}
    return true;
}

//move a vertex/resize a region
//we know a point is selected. We move that point to the location of mousePt (or do nothing if they have the same location)
//if forcing rectangularity, it's slightly more complicated as we adjust the other points logically to maintain a rectangle
template <class T_Region>
bool RegionCollection<T_Region>::Callback_Point_Move_Check(const cv::Point2f& mousePt, int event, int flags)
{
    //check if we actually need to move the point
    bool hasChanged = false;
    if (*(pSelectedRegion->pSelectedPt) != mousePt )
    {
        pSelectedRegion->Move_Point(mousePt);
        hasChanged = true;
    }

    //'place' point (i.e. stop moving/editing) when user releases left click. Note: this doesn't alter the point or region
	if (event == CV_EVENT_LBUTTONUP)
	{
        if (verbose)  { std::cout << "Placed point: " << *pSelectedRegion->pSelectedPt << std::endl;}
        pSelectedRegion->pSelectedPt = nullptr;
		//pSelectedPt = nullptr;
		
		regionEditAction = REGION_EDIT_ACTION::EDITING;
	}
    return hasChanged;
}

//delete all existing regions
template <class T_Region>
void RegionCollection<T_Region>::Clear(void)
{
    for (T_Region* pRegion : regions)	delete pRegion;  //###this is almost certianly bad but we've reached beyond my understanding of c++###
    regions.clear();                //### actually, I think it just wants me to make Region::~Region (the destructor) virtual? but the destructors for region and statisticsregion should be defined and distinct anyway...
}

//create a new region
template <class T_Region>
bool RegionCollection<T_Region>::Callback_New_Region_Definition(const cv::Point2f& mousePt, int event, int flags)
{
	for (T_Region* pRegion : regions)  { pRegion->isSelected = false; }   //deselect any selected regions (right-click will override anything else going on)
	regionEditAction = REGION_EDIT_ACTION::CREATING_NEW_REGION;

    if (!allowMultipleRegions)  this->Clear();				//if not allowing multiple regions, there's still an existing region vector but it'll never have > length 1
	
    T_Region* pNewRegion = new T_Region( cv::Rect(mousePt, mousePt+cv::Point2f(1,1)) );  //initialize rectangle to 1x1 square slightly offset from pointer, but it'll update as dragged
    regions.push_back( pNewRegion );
	pNewRegion->isSelected = true;
	pSelectedRegion = pNewRegion;
    pNewRegion->allowNonrectangularity = allowNonrectangularity;  //should/could use setter functions (Set_Allow_Nonrectangularity)
	pNewRegion->imgSize = imgSize;

    movingRefPt = mousePt;

	if (verbose)  { std::cout << "Creating new region: " << pSelectedRegion->regionLabel << std::endl; }
    return true;  //this always creates a new region and potentially deselects some other regions, so stuff is changed
}

//select/deselect a/multiple regions
template <class T_Region>
bool RegionCollection<T_Region>::Callback_Region_Selection(const cv::Point2f& mousePt, int event, int flags)
{
    bool hasChanged = false;

	//if holding ctrl, allow selecting multiple regions. Otherwise clear out prior selection if any
	if (!(flags & CV_EVENT_FLAG_CTRLKEY))
	{
        pSelectedRegion = nullptr;
		for (T_Region* pRegion : regions)
		{
            if (pRegion->isSelected)
            {
                pRegion->isSelected = false;
                if (verbose)  { std::cout << "De-selected region: " << pRegion->regionLabel << std::endl; }
                hasChanged = true;
            }
		}
	}

    //loop thru the existing regions and see if clicked inside it. If so, select the region
	for (T_Region* pRegion : regions)
	{
		if (pRegion->Contains(mousePt))
		{
            pSelectedRegion = pRegion;  //###this could a problem?
			regionEditAction = REGION_EDIT_ACTION::EDITING;
			pRegion->isSelected = true;
			if (verbose)  { std::cout << "Selected region: " << pRegion->regionLabel << std::endl; }
            return true;    //note this quits immediately if it finds a region. What if there are multiple regions stacked on top of one another?
		}
	}

    return hasChanged;  //no new region was selected, however some region(s) were deselected
}

//select/deslect a vertex
template <class T_Region>
bool RegionCollection<T_Region>::Callback_Point_Selection(const cv::Point2f& mousePt, int event, int flags)
{
    cv::Point2f* pClosestPt = nullptr;
	float distSqr, bestDistSqr = pointSelectionThreshold;

    //find the closest point to the click location within pointSelectionThreshold pixels. If no nearby point, pClosestPt will still be nullptr after this loop
    for (cv::Point2f& polyBoundPt : pSelectedRegion->polyBound)
	{
        distSqr = Sqrd_Dist(mousePt, polyBoundPt);
        if (distSqr < bestDistSqr)  { bestDistSqr = distSqr;  pClosestPt = &polyBoundPt; }
	}

    //if there was a point near the click, then select it. Else do nothing
    if (pClosestPt != nullptr)
	{
		regionEditAction = REGION_EDIT_ACTION::MOVING_POINT;
        pSelectedRegion->pSelectedPt = pClosestPt;  //pointer to the actual cv::Point in region.polyBound vector. Can only select 1 point at a time.
        if (verbose)  { std::cout << "Selected point: " << *pClosestPt << std::endl; }
        return true;  //a point was selected
	}
    return false;  //no point was selected
}




template <class T_Region>
void RegionCollection<T_Region>::Set_Allow_Multiple_Regions(bool allow)
{
	allowMultipleRegions = allow;
}

template <class T_Region>
void RegionCollection<T_Region>::Set_Allow_Nonrectangularity(bool allow)
{
	allowNonrectangularity = allow;
}

//Maybe make these all non-static at some point if it meshes with the resizing business.
template <class T_Region>
void RegionCollection<T_Region>::Set_Image_Size(cv::Size _imgSize)
{
	imgSize = _imgSize;
}

template <class T_Region>
cv::Size RegionCollection<T_Region>::Get_Image_Size(void)
{
    return imgSize;
}

template <class T_Region>
bool RegionCollection<T_Region>::Get_Allow_Multiple_Regions(void)
{
    return allowMultipleRegions;
}

template <class T_Region>
bool RegionCollection<T_Region>::Get_Allow_Nonrectangularity(void)
{
    return allowNonrectangularity;
}
