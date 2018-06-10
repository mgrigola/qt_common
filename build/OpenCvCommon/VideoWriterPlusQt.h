#pragma once
#include "opencv2/videoio.hpp"
#include <vector>
#include <string>
#include <queue>
#include <thread>
#include <chrono>
#include <iostream>
#include <mutex>

class VideoWriterPlusStream :
	public cv::VideoWriter
{
public:
	std::string fileName;
	int codec;
	double fps, quality;
	cv::Size imgSize;
	bool isColor;
    cv::Mat* frameSource;
    bool usingFixedSource;

	~VideoWriterPlusStream(void) {}
	VideoWriterPlusStream(void) {}
	VideoWriterPlusStream(const std::string& _fileName, int _codec, double _fps, double _quality = 100.0);
    VideoWriterPlusStream(const std::string& _fileName, int _codec, double _fps, cv::Mat* _frameSource);
	void Initialize(cv::Size& _imgSize, bool _isColor);
};

class VideoWriterPlusQt
{
private:
	bool bufferDone, stopSave;
    std::mutex mtxWriteBuffer;

public:
	std::vector <VideoWriterPlusStream> streams;
	std::queue <std::vector <cv::Mat>> writeBuffer;
	int codec;
	double fps, quality;

    VideoWriterPlusQt(const std::string& _codec = std::string("H264"), double _fps = 30.0, double _quality = 100.0);
    VideoWriterPlusQt(int _codec = cv::VideoWriter::fourcc('H','2','6','4'), double _fps = 30.0, double _quality = 100.0);
    ~VideoWriterPlusQt(void);
	void Start_Save(void);
	void Stop_Save(bool waitTilDone=true);
	void Add_Stream(const std::string& _fileName);
	void Add_Stream(const std::string& _fileName, int _codec, double _fps, double _quality);
    void Add_Stream(const std::string& _fileName, cv::Mat* _frameSource);  //fixed source mat test
	void Remove_Streams(void);
	void Buffer_Thread(void);
	void Write_Frame(const cv::Mat& image1 = cv::Mat(), const cv::Mat& image2 = cv::Mat(), const cv::Mat& image3 = cv::Mat(), const cv::Mat& image4 = cv::Mat());
    void Write_Frame_Fixed(void);  //fixed source mat test
    static int Convert_Codec_String_To_Cv_Enum(const std::string& codecString);  //these stupid codec numbers... cv::VideoWriter::fourcc('H','2','6','4') = VideoWriterPlusQt::Convert_Codec_String_To_Cv_Enum("H264");
};

