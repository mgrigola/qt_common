#include "VideoWriterPlusQt.h"

VideoWriterPlusStream::VideoWriterPlusStream(const std::string& _fileName, int _codec, double _fps, double _quality) :
	fileName(_fileName),
	codec(_codec),
	fps(_fps),
    quality(_quality),
    usingFixedSource(false)
{
}

//fixed source mat test
VideoWriterPlusStream::VideoWriterPlusStream(const std::string& _fileName, int _codec, double _fps, cv::Mat* _frameSource) :
  fileName(_fileName),
  codec(_codec),
  fps(_fps),
  quality(99999),
  usingFixedSource(true),
  frameSource(_frameSource)
{
}

void VideoWriterPlusStream::Initialize(cv::Size& _imgSize, bool _isColor)
{
	imgSize = _imgSize;
	isColor = _isColor;
	open(fileName, codec, fps, imgSize, isColor);
    set(cv::VIDEOWRITER_PROP_QUALITY, 100);  //I don't think this does anything... quality is always pretty lossy
}



VideoWriterPlusQt::VideoWriterPlusQt(int _codec, double _fps, double _quality) :
	codec(_codec),
	fps(_fps),
	quality(_quality)
{
	streams.reserve(4);
}

VideoWriterPlusQt::~VideoWriterPlusQt(void)
{
}


//***Self: please create a class VideoWriterStream and reorganize this to handle arbitrary number of streams
void VideoWriterPlusQt::Write_Frame(const cv::Mat& image1, const cv::Mat& image2, const cv::Mat& image3, const cv::Mat& image4)
{
    mtxWriteBuffer.lock();
	writeBuffer.push(std::vector<cv::Mat>(4));
    mtxWriteBuffer.unlock();
    //std::cout << "writeBuffer size" << writeBuffer.size() << " + " << pFrame << std::endl;
	std::vector<cv::Mat>* pFrame = &writeBuffer.back();
	if (!image1.empty())	image1.copyTo(pFrame->at(0));
	if (!image2.empty())	image2.copyTo(pFrame->at(1));
	if (!image3.empty())	image3.copyTo(pFrame->at(2));
	if (!image4.empty())	image4.copyTo(pFrame->at(3));
	//std::cout << "writeBuffer size" << writeBuffer.size() << std::endl;
}

//fixed source mat test
void VideoWriterPlusQt::Write_Frame_Fixed(void)
{
    mtxWriteBuffer.lock();
    writeBuffer.push(std::vector<cv::Mat>(streams.size()));
    std::vector<cv::Mat>* pFrame = &writeBuffer.back();
    mtxWriteBuffer.unlock();

    //would be nice if this was more generic and not tied to fixed positions in writeBuffer vector, but order could be messed up and crash if reallocated? (like, use push_back)
    for (size_t streamNo=0; streamNo < streams.size(); streamNo++)
    {
        if ( !streams[streamNo].frameSource->empty() )   streams[streamNo].frameSource->copyTo(pFrame->at(streamNo) );
    }
    //std::cout << "writeBuffer size" << writeBuffer.size() << std::endl;
}

void VideoWriterPlusQt::Add_Stream(const std::string& _fileName)
{
	streams.push_back( VideoWriterPlusStream(_fileName, codec, fps, quality) );
}

void VideoWriterPlusQt::Add_Stream(const std::string& _fileName, int _codec, double _fps, double _quality)
{
	streams.push_back( VideoWriterPlusStream(_fileName, _codec, _fps, _quality) );
}

//fixed source mat test (specify the mat to write when adding the stream, then dont need to specify for each frame). Will break if mat address changes...
void VideoWriterPlusQt::Add_Stream(const std::string& _fileName, cv::Mat* _frameSource)
{
    streams.push_back( VideoWriterPlusStream(_fileName, codec, fps, _frameSource) );
}

void VideoWriterPlusQt::Remove_Streams()
{
	streams.clear();
}

void VideoWriterPlusQt::Start_Save()
{
	stopSave = false;
	//for (int pos=0; pos<bufferSize; pos++)  { bufferReady[pos] = false; }
    std::thread bufferThread(&VideoWriterPlusQt::Buffer_Thread, this);
	bufferThread.detach();  //branches the execution: current thread continues about its business. Buffer_Thread operates independently
}

//waitTilDone: sleep this thread (should be the main processing thread) until the buffer finishes writing all its frames. Pretty sure necessary to prevent crash if program exits and releases all memory before buffer finishes.
void VideoWriterPlusQt::Stop_Save(bool waitTilDone)
{
	stopSave = true;
	if (waitTilDone)
	{
		while(!bufferDone)
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}

//crude video buffering? Run this guy in a separate thread, then call Get_Frame from your processing loop -> then your processing doesn't need to wait on camera/disk to grab the image, it's already ready.
//Returns false when the thread can exit (no data left to read). Else returns true even if it didn't do anything.
void VideoWriterPlusQt::Buffer_Thread()
{
	size_t bufferSize;
	size_t streamCount;
    //int frameCount = 0;
	bufferDone = false;
    cv::Size streamCvSize;

	while(true)
	{
        //mtxWriteBuffer.lock();
		bufferSize = writeBuffer.size();
		if (stopSave)
		{
			if (bufferSize == 0)			//save is stopped and buffer is empty - done
            {
                //mtxWriteBuffer.unlock();
                break;
            }
		}
        else if (bufferSize < 2)  //used to be some issue here but i think is resolved by the mutex. Can try increasing to < 3 if bizarre crashing...
		{
            //mtxWriteBuffer.unlock();
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			continue;
		}

		//std::cout << "Writing frame: " << frameCount++ << "       writer buffer = " << bufferSize << std::endl;

		//std::vector<cv::Mat> thisFrame = writeBuffer.front();
		//for (int vidNo=0; vidNo < vidCount; vidNo++)
		//{
		//	if (thisFrame[vidNo].empty())
		//		continue;

		//	if (!vWriters[vidNo].isOpened())  vWriters[vidNo].open(fileNames[vidNo], codec, fps, cv::Size(thisFrame[vidNo].cols, thisFrame[vidNo].rows), vidNo!=2);
		//	vWriters[vidNo].write(thisFrame[vidNo]);
		//}
		streamCount = streams.size();
		std::vector<cv::Mat> *pFrame = &writeBuffer.front();

        //mtxWriteBuffer.unlock();
        for (size_t streamNo=0; streamNo < streamCount; streamNo++)
		{
            if (pFrame->at(streamNo).empty())
				continue;

            if (!streams[streamNo].isOpened())
            {
                streamCvSize.width = pFrame->at(streamNo).cols;
                streamCvSize.height = pFrame->at(streamNo).rows;
                streams[streamNo].Initialize(streamCvSize, pFrame->at(streamNo).channels()!=1 );
            }
            streams[streamNo].write(pFrame->at(streamNo));
		}

        mtxWriteBuffer.lock();
        writeBuffer.pop();
        mtxWriteBuffer.unlock();
	}

	bufferDone = true;
    //cv::VideoWriter should automatically release in its destructor
//    for (int streamNo=0; streamNo < streamCount; streamNo++)
//        if ( streams[streamNo].isOpened() )
//            streams[streamNo].release();
}


int VideoWriterPlusQt::Convert_Codec_String_To_Cv_Enum(const std::string& codecString)
{
    if (codecString.size()!=4)  return 0;
    return cv::VideoWriter::fourcc(codecString[0],codecString[1],codecString[2],codecString[3]);
}
