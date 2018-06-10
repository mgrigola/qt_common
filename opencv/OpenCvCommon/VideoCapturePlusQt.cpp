#include "VideoCapturePlusQt.h"

VideoCapturePlusQt::~VideoCapturePlusQt()
{
    Stop_Read();
}

VideoCapturePlusQt::VideoCapturePlusQt(void) :
    bufferThreadIsRunning(false),
    frameChanged(false),
    stopReading(false),
    stopBuffering(false),
    isCamFeed(false)
{ }

//might call this when running from camera when want to quit but still process remaining frames? Doesn;t actually handle that now but it could some day.
void VideoCapturePlusQt::Stop_Read(bool _hardStop)
{
    stopReading = true;
    stopBuffering = _hardStop;
    while (bufferThreadIsRunning)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

    release();
}

VideoCapturePlusQt::VideoCapturePlusQt(const char* _fileName, const char* _defaultFileName, int _firstFrame, int _verbocity, int _camNo)
{
    Start_Read(_fileName, _defaultFileName, _firstFrame, _verbocity, _camNo);
}

bool VideoCapturePlusQt::Start_Read(const char* _fileName, const char* _defaultFileName, int _firstFrame, int _verbocity, int _camNo)
{
    verbocity = _verbocity;
    isCamFeed = false;
    bufferThreadIsRunning = false;
    while (vidBuffer.size()>0)  { vidBuffer.pop(); }  //maybe duplicative now, but clear the buffer upon opening a new file

    //Try to open the given file - i.e. use file from command line parameter if passed
    if (_fileName!= nullptr && _fileName[0] != '\0')
    {
        if ( verbocity>0 )  {std::cout << "opening input file: " << _fileName << std::endl;}
        open(_fileName);
        Set_File_And_Dir(_fileName);
    }
    //If couldn't open the given file or no file was given, open the default file - i.e. hardcode a test file into the program to use unless something else is explicitly specified
    if (!isOpened() && _defaultFileName != nullptr && _defaultFileName[0] != 0)
    {
        if ( verbocity>0 )  {std::cout << "opening default file:  " << _defaultFileName << std::endl;}
        open(_defaultFileName);
        Set_File_And_Dir(_defaultFileName);
    }
    if (!isOpened() && _camNo >= 0)
    {
        if ( verbocity>0 )  {std::cout << "getting live feed from camera" << std::endl;}
        open(_camNo);
        isCamFeed = true;
        openFileDir = "";
        std::string timestamp;
        Get_Timestamp(timestamp);
        openFileName = "Camera feed - " + timestamp;
    }
    if (!isOpened())
    {
        isOkay = false;
        if ( verbocity > 0)  { std::cout << "Couldn't open anything. Exiting..." << std::endl; }
        return false;
    }

    if (isCamFeed)
    {
        firstFrame = 0;
        frameNo = 0;
        frameCount = INT_MAX;
        cv::Mat image;
        this->read(image);
        size = image.size();
        type = image.type();
        Set_Camera_Settings();
        if (verbocity == 3) Test_Cam_Settings();
    }
    else
    {
        firstFrame = _firstFrame;
        frameNo = firstFrame;
        frameCount = static_cast<int>(this->get(cv::CAP_PROP_FRAME_COUNT));
        this->set(cv::CAP_PROP_POS_FRAMES, firstFrame);
        size = cv::Size(static_cast<int>(this->get(cv::CAP_PROP_FRAME_WIDTH)),static_cast<int>(this->get(cv::CAP_PROP_FRAME_HEIGHT)));
    }


    int iCodec = static_cast<int>(this->get(cv::CAP_PROP_FOURCC));
    char capCodec[] = {(char)(iCodec & 0XFF),(char)((iCodec & 0XFF00) >> 8),(char)((iCodec & 0XFF0000) >> 16),(char)((iCodec & 0XFF000000) >> 24),0};  //cap.get returns a double that encodes the codec string
    codec = capCodec;
    fps = this->get(cv::CAP_PROP_FPS);
    cpuFreq = cv::getTickFrequency();
    startTicks = 0;
    lastTime = 0;

    if ( verbocity>0 )
    {
        std::cout << "frame size: " << size << std::endl;
        std::cout << "frame rate: " << fps << std::endl;
        std::cout << "codec: " << codec << std::endl;
    }


    badFrame = false;
    isOkay = true;

    isPaused = false;
    stopBuffering = false;			//stop buffering when want to quit smoothly - continue to process and empty what's already in the buffer
    pauseBuffering = false;			//pause buffering when altering the buffer and such. i.e. when jumping to a different frame pause first, then clear buffer, then start again from new frame 			//used to be true but I forgot why...
    pauseReading = false;			//pause reading when altering the buffer and wait for the buffer to update before reading next frame
    stopReading = false;			//'hard stop' - stop reading when want to quit immediately. We'll discard anything in the buffer
    bufferFrameNo = frameNo;
    bufferOverload = false;
    bufferSize = 8;
    bufferThread = std::thread(&VideoCapturePlusQt::Buffer_Thread, this);
//	HANDLE pThreadHnd = bufferThread.native_handle();
//	SetThreadPriority(pThreadHnd, THREAD_PRIORITY_TIME_CRITICAL);
    bufferThread.detach();

    return true;
}

//Take the full/absolute file name and parse out the directory and the file name
void VideoCapturePlusQt::Set_File_And_Dir(const std::string& _fileName)
{
    openFileName = _fileName;
    size_t a = openFileName.rfind("\\"); a = (a==std::string::npos) ? 0 : a;
    size_t b = openFileName.rfind("/");  b = (b==std::string::npos) ? 0 : b;
    size_t lastSlash = std::max(a, b);
    size_t lastDot = openFileName.rfind(".");
    openFileDir = openFileName.substr(0, lastSlash)+"/";
    openFileName = openFileName.substr(lastSlash+1, lastDot-lastSlash-1);
}

//crude video buffering? Run this guy in a separate thread, then call Get_Frame from your processing loop -> then your processing doesn't need to wait on camera/disk to grab the image, it's already ready.
//Returns false when the thread can exit (no data left to read). Else returns true even if it didn't do anything.
void VideoCapturePlusQt::Buffer_Thread()
{
    double timestampMsec;
    startTicks = cv::getTickCount();
    bufferThreadIsRunning = true;

    while(true)
    {
        //user exited the program? Quit immediately
        if (stopReading)
            break;

        //
        if (pauseBuffering || stopBuffering)
        {
            bufferThreadIsRunning = false;
            //std::cout << "Buffering is paused" << std::endl;

//            while(pauseBuffering)
//                std::this_thread::sleep_for(std::chrono::milliseconds(10));

            for (size_t cnt=0; cnt<vidBuffer.size(); cnt++) { vidBuffer.pop(); }
            //std::cout << "Buffer Cleared" << std::endl;

            if (frameChanged)
            {
                pauseBuffering = false;
                bufferThreadIsRunning = true;
                this->set(cv::CAP_PROP_POS_FRAMES, newFrameNo);
            }

            continue;
        }

        //all remaining frames in video are in buffer - buffering thread can exit.
        bufferFrameNo = frameNo + vidBuffer.size();
        if (bufferFrameNo >= frameCount)
        {
            break;
        }

        //the file reading/buffering is running faster than the processing. If camera feed, we have to keep expanding the buffer to avoid dropping frames. If file feed, there's no work to do, so sleep for a bit until processing catches up
        if (vidBuffer.size() >= bufferSize)
        {
            //if buffer is overloaded (cam feed), we're either going to drop frames or we need to end collection now (we'll continue to process the frames currently in the buffer) and start a new collection. Ideally we'd keep saving images to disk...
            if (bufferOverload)
                break;

            if (isCamFeed)
            {
                bufferOverload = !Set_Buffer_Size(2*bufferSize);
                if ( verbocity>1 ) std::cout << "Buffer size: " << bufferSize << std::endl;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(1));			//maybe only do this if not cam feed?
            continue;
        }

        //if (startTicks==0) startTicks = cv::getTickCount();

        //hopefully typical case: we're ready to buffer more data. Read+buffer a frame, mark buffer element ready (for retrieval/processing)
        vidBuffer.push(cv::Mat());
        this->read(vidBuffer.back());  //write directly to the queue - notice we added the element before populating it, so reader cannot read the last element (it's initially empty when size() increased)
        if ( verbocity>2 && bufferFrameNo < 8 ) std::cout << "Channels: " << vidBuffer.back().channels() << "  Type: " << vidBuffer.back().type() << "  depth: " << vidBuffer.back().depth() << std::endl;

        //double test = this->get(cv::CAP_PROP_POS_MSEC);
        if (isCamFeed) timestampMsec = 1000.0*static_cast<double>(cv::getTickCount() - startTicks)/cpuFreq;   //otherwise we'll record the time at which the camera was polled (in msec, with 0 roughly when the first frame captured).
        else  timestampMsec = 1000.0*static_cast<double>(bufferFrameNo)/fps;
        timestampBuffer.push(timestampMsec);
    }

    std::cout << "bufferThread has stopped" << std::endl;
    bufferThreadIsRunning = false;
    //Stop_Read();
//    while (vidBuffer.size() > 0)
//        vidBuffer.pop();
}

bool VideoCapturePlusQt::Set_Buffer_Size(int _bufferSize)
{
    if (!bufferThreadIsRunning)
        std::cout << "mmkay, Imma go ahead and crash" << std::endl;

    if (_bufferSize > MAX_VCP_BUFFER_SIZE)
    {
        bufferSize = MAX_VCP_BUFFER_SIZE;
        return false;
    }

    bufferSize = _bufferSize;
    return true;
}


bool VideoCapturePlusQt::Get_Frame(cv::Mat& image, double& timestampMsec, bool advanceFrame)
{
    if (frameNo >= frameCount)
    {
        isOkay = false;
        return false;
    }

    if (stopBuffering || pauseBuffering)
        return false;

    //we're ready to retrieve data. Buffer is running faster than processing
    if (vidBuffer.size() > 1 || ( (stopReading || pauseReading || !bufferThreadIsRunning) && vidBuffer.size()==1) )          // >1 because otherwise we could start reading from the last element before we're done writing, disaster. Once buffer is stopped we'll read the last element.
    {
        image = vidBuffer.front();
        timestampMsec = timestampBuffer.front();
        vidBuffer.pop();                //does this destroy the mat?
        timestampBuffer.pop();
        if ( verbocity>2 )
            std::cout << "Frame: " << frameNo << " @ " << timestampMsec << "+" << (timestampMsec-lastTime) << "  buf: " << vidBuffer.size() << " sz: " << image.cols << "x" << image.rows << "x" << image.channels() << std::endl;
        lastTime = timestampMsec;
    }
    else								//processing is running faster than image read. wait for buffer to get more frames
    {
        if (bufferOverload || stopReading)
            isOkay = false;

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        return false;
    }

    if (image.empty())
    {
        if (badFrame)  { isOkay = false;}
        badFrame = true;
        if ( verbocity>0 )  {std::cout << "bad frame: " << frameNo << std::endl;}
        return false;
    }

    if (advanceFrame)  ++frameNo;
    return true;
}

bool VideoCapturePlusQt::Get_Frame(cv::Mat& rawImage, cv::Mat& grayImage, cv::Mat& grayImageUC3, double& timestampMsec, bool advanceFrame)
{
    //pauseBuffering = false;			//used to be used in conjunction with initializing with buffer paused, but I forgot why...
    if (!this->Get_Frame(rawImage, timestampMsec, advanceFrame))
        return false;


    if (rawImage.channels() == 1)
    {
        rawImage.copyTo(grayImage);
        cv::cvtColor(grayImage, grayImageUC3, CV_GRAY2BGR);
    }
    else
    {
        cv::cvtColor(rawImage, grayImage, CV_BGR2GRAY);
        cv::cvtColor(grayImage, grayImageUC3, CV_GRAY2BGR);
    }

    return true;
}

void VideoCapturePlusQt::Set_Frame_No(int _frameNo)
{
    pauseBuffering = true;
    frameChanged = true;
    newFrameNo = _frameNo;

//    while (bufferThreadIsRunning)
//        std::this_thread::sleep_for(std::chrono::milliseconds(10));

    //std::cout << "Setting file read position" << std::endl;
    //this->set(cv::CAP_PROP_POS_FRAMES, _frameNo);
    frameNo = _frameNo;
    bufferFrameNo = _frameNo;
    //pauseBuffering = false;
}

void VideoCapturePlusQt::Test_Cam_Settings()
{
    std::cout << "CAP_PROP_APERTURE: " << this->get(cv::CAP_PROP_APERTURE) << std::endl;
    std::cout << "CAP_PROP_AUTOFOCUS: " << this->get(cv::CAP_PROP_AUTOFOCUS) << std::endl;
    std::cout << "CAP_PROP_AUTO_EXPOSURE: " << this->get(cv::CAP_PROP_AUTO_EXPOSURE) << std::endl;
    std::cout << "CAP_PROP_APERTURE: " << this->get(cv::CAP_PROP_BRIGHTNESS) << std::endl;
    std::cout << "CAP_PROP_BUFFERSIZE: " << this->get(cv::CAP_PROP_BUFFERSIZE) << std::endl;
    std::cout << "CAP_PROP_CONTRAST: " << this->get(cv::CAP_PROP_CONTRAST) << std::endl;
    std::cout << "CAP_PROP_CONVERT_RGB: " << this->get(cv::CAP_PROP_CONVERT_RGB) << std::endl;
    std::cout << "CAP_PROP_EXPOSURE: " << this->get(cv::CAP_PROP_EXPOSURE) << std::endl;
    std::cout << "CAP_PROP_FOCUS: " << this->get(cv::CAP_PROP_FOCUS) << std::endl;
    std::cout << "CAP_PROP_FPS: " << this->get(cv::CAP_PROP_FPS) << std::endl;
    std::cout << "CAP_PROP_GAIN: " << this->get(cv::CAP_PROP_GAIN) << std::endl;
    std::cout << "CAP_PROP_GAMMA: " << this->get(cv::CAP_PROP_GAMMA) << std::endl;
    std::cout << "CAP_PROP_GUID: " << this->get(cv::CAP_PROP_GUID) << std::endl;
    std::cout << "CAP_PROP_HUE: " << this->get(cv::CAP_PROP_HUE) << std::endl;
    std::cout << "CAP_PROP_ISO_SPEED: " << this->get(cv::CAP_PROP_ISO_SPEED) << std::endl;
    std::cout << "CAP_PROP_MODE: " << this->get(cv::CAP_PROP_MODE) << std::endl;
    std::cout << "CAP_PROP_MONOCHROME: " << this->get(cv::CAP_PROP_MONOCHROME) << std::endl;
    std::cout << "CAP_PROP_OPENNI_FOCAL_LENGTH: " << this->get(cv::CAP_PROP_OPENNI_FOCAL_LENGTH) << std::endl;
    std::cout << "CAP_PROP_OPENNI_OUTPUT_MODE: " << this->get(cv::CAP_PROP_OPENNI_OUTPUT_MODE) << std::endl;
    std::cout << "CAP_PROP_OPENNI_MAX_BUFFER_SIZE: " << this->get(cv::CAP_PROP_OPENNI_MAX_BUFFER_SIZE) << std::endl;
    std::cout << "CAP_PROP_POS_FRAMES: " << this->get(cv::CAP_PROP_POS_FRAMES) << std::endl;
    std::cout << "CAP_PROP_POS_MSEC: " << this->get(cv::CAP_PROP_POS_MSEC) << std::endl;
    std::cout << "CAP_PROP_PVAPI_PIXELFORMAT: " << this->get(cv::CAP_PROP_PVAPI_PIXELFORMAT) << std::endl;
    std::cout << "CAP_PROP_TEMPERATURE: " << this->get(cv::CAP_PROP_TEMPERATURE) << std::endl;
    std::cout << "CAP_PROP_TRIGGER: " << this->get(cv::CAP_PROP_TRIGGER) << std::endl;
    std::cout << "CAP_PROP_TRIGGER_DELAY: " << this->get(cv::CAP_PROP_TRIGGER_DELAY) << std::endl;
    std::cout << "CAP_PROP_XI_ACQ_BUFFER_SIZE: " << this->get(cv::CAP_PROP_XI_ACQ_BUFFER_SIZE) << std::endl;
    std::cout << "CAP_PROP_WHITE_BALANCE_BLUE_U: " << this->get(cv::CAP_PROP_WHITE_BALANCE_BLUE_U) << std::endl;
    std::cout << "CAP_PROP_XI_AUTO_BANDWIDTH_CALCULATION: " << this->get(cv::CAP_PROP_XI_AUTO_BANDWIDTH_CALCULATION) << std::endl;
    std::cout << "CAP_PROP_XI_GAIN: " << this->get(cv::CAP_PROP_XI_GAIN) << std::endl;
    std::cout << "CAP_PROP_XI_FRAMERATE: " << this->get(cv::CAP_PROP_XI_FRAMERATE) << std::endl;
    std::cout << "CAP_PROP_ZOOM: " << this->get(cv::CAP_PROP_ZOOM) << std::endl;
    std::cout << "CAP_PROP_SETTINGS: " << this->get(cv::CAP_PROP_SETTINGS) << std::endl;

    this->set(cv::CAP_PROP_AUTO_EXPOSURE, 0);
    this->set(cv::CAP_PROP_EXPOSURE, -8);
    std::cout << "Setting - CAP_PROP_SETTINGS: " << this->set(cv::CAP_PROP_SETTINGS, 0.0) << std::endl;  //might bring up a window to set common settings via gui


}

void VideoCapturePlusQt::Set_Camera_Settings(void)
{
    this->set(cv::CAP_PROP_FRAME_HEIGHT, 1080);
    this->set(cv::CAP_PROP_FRAME_WIDTH, 1920);
    this->set(cv::CAP_PROP_FPS, 30);
    this->set(cv::CAP_PROP_EXPOSURE, 0.025);
    //_sleep(1000);
}


bool VideoCapturePlusQt::Toggle_Pause_Buffering(void)
{
    stopBuffering = !stopBuffering;
    return stopBuffering;
}

bool VideoCapturePlusQt::Set_Pause_Buffering(bool pause)
{
    stopBuffering = pause;
    return stopBuffering;
}
