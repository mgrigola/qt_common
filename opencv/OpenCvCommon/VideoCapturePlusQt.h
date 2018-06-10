#ifndef VIDEOCAPTUREPLUSQT_H
#define VIDEOCAPTUREPLUSQT_H

#include "opencv2/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/videoio.hpp"
#include "opencv2/video.hpp"
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <queue>
#include <windows.h>
#include "commonFunctions.h"

#define MAX_VCP_BUFFER_SIZE 512  //if you have a camera reading faster than we can process, the buffer will keep growing over time. At some point we need to stop - we only have so much memory. Either pause input for a bit or exit or slow down or just write extra stuff to file to process later. Problem = unsolved

class VideoCapturePlusQt :
    public cv::VideoCapture
{
private:
    void Buffer_Thread(void);
    std::queue <cv::Mat> vidBuffer;   //FIFO queue: we want to avoid waiting on input (or losing camera frames) during processing loop. So always store the next few frames to process.
    std::queue <double> timestampBuffer;
    size_t bufferFrameNo, bufferSize, verbocity, newFrameNo;
    bool stopReading, pauseReading, stopBuffering, pauseBuffering, bufferOverload, bufferThreadIsRunning, frameChanged;
    std::thread bufferThread;

    double cpuFreq, lastTime;
    int64 startTicks;
    bool badFrame;
    void Set_File_And_Dir(const std::string& _fileName);
    void Test_Cam_Settings(void);
    void Set_Camera_Settings(void);

public:
    std::string openFileName, openFileDir, codec;
    cv::Size size;
    size_t frameCount, firstFrame, frameNo;
    int type;
    bool isOkay, isCamFeed, isPaused;
    double fps;

    VideoCapturePlusQt(void);
    ~VideoCapturePlusQt(void);
    VideoCapturePlusQt(const char* _fileName, const char* _defaultFileName=nullptr, int _firstFrame=0, int _verbose=0, int camNo=-1);
    bool Start_Read(const char* _fileName, const char* _defaultFileName=nullptr, int _firstFrame=0, int _verbocity=0, int camNo=-1);
    void Stop_Read(bool _hardStop=false);
    bool Get_Frame(cv::Mat& image, double& timestamp, bool advanceFrame=true);
    bool Get_Frame(cv::Mat& rawImage, cv::Mat& grayImage, cv::Mat& grayImageUC3, double& timestamp, bool advanceFrame=true);
    void Set_Frame_No(int _frameNo);
    bool Set_Buffer_Size(int _bufferSize);
    bool Toggle_Pause_Buffering(void);
    bool Set_Pause_Buffering(bool pause);
};

#endif // VIDEOCAPTUREPLUSQT_H
