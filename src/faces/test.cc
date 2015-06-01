#include "opencv2/video/tracking.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/objdetect/objdetect.hpp"


#include <iostream>
#include <ctype.h>

using namespace cv;
using namespace std;

Mat image;
int trackObject = 0;
Rect selection;

int main()
{
VideoCapture cap;
Rect trackWindow;
int hsize = 16;
float hranges[] = {0,180};
const float* phranges = hranges;
int matchesNum = 0;
CascadeClassifier cascade;
if (!cascade.load("/usr/local/share/OpenCV/haarcascades/haarcascade_frontalface_default.xml")) {
    cout << "Cannot load face xml!" << endl;
    return -1;
}


cap.open("/home/meriem/work/repere/data/BFMTV_BFMStory_2011-11-28_175800.MPG");

if (!cap.isOpened()) {
    cout << "***Could not initialize capturing...***\n";
    return -1;
}

namedWindow( "Result", 1 );

Mat frame, hsv, hue, hist, mask, backproj;

for(;;)
{

    cap >> frame;
    if( frame.empty() )
        break;

    frame.copyTo(image);

    if ( !trackObject )
    {
        Mat grayframe; 
        vector <Rect> facesBuf;
        int detectionsNum = 0;

        cvtColor(image, grayframe, CV_BGR2GRAY);
        cascade.detectMultiScale(grayframe, facesBuf, 1.2, 4, CV_HAAR_FIND_BIGGEST_OBJECT |
        CV_HAAR_SCALE_IMAGE, cvSize(0, 0));

        detectionsNum = (int) facesBuf.size();
        Rect *faceRects = &facesBuf[0];

        //It must found faces in three consecutives frames to start the tracking to discard false positives
        if (detectionsNum > 0) 
            matchesNum += 1;
        else matchesNum = 0;
        if ( matchesNum == 3 )
        {
            trackObject = -1;
            selection = faceRects[0];
        }

        for (int i = 0; i < detectionsNum; i++) 
        { 
            Rect r = faceRects[i];
            rectangle(image, Point(r.x, r.y), Point(r.x + r.width, r.y + r.height), CV_RGB(0, 255, 0)); 
        }   
    }

    if( trackObject )
    {
        cvtColor(image, hsv, CV_BGR2HSV);
        inRange(hsv, Scalar(0, 69, 53),
                    Scalar(180, 256, 256), mask);
        int ch[] = {0, 0};
        hue.create(hsv.size(), hsv.depth());
        mixChannels(&hsv, 1, &hue, 1, ch, 1);

        if( trackObject < 0 )
        {
            Mat roi(hue, selection), maskroi(mask, selection);
            calcHist(&roi, 1, 0, maskroi, hist, 1, &hsize, &phranges);
            normalize(hist, hist, 0, 255, CV_MINMAX);

            trackWindow = selection;
            trackObject = 1;      
        }

        calcBackProject(&hue, 1, 0, hist, backproj, &phranges);
        backproj &= mask;
        RotatedRect trackBox = CamShift(backproj, trackWindow,
                TermCriteria( CV_TERMCRIT_EPS | CV_TERMCRIT_ITER, 10, 1 ));
        if( trackWindow.area() <= 1 )
        {
            int cols = backproj.cols, rows = backproj.rows, r = (MIN(cols, rows) + 5)/6;
            trackWindow = Rect(trackWindow.x - r, trackWindow.y - r,
                    trackWindow.x + r, trackWindow.y + r) &
                    Rect(0, 0, cols, rows);
        }

        ellipse( image, trackBox, Scalar(0,0,255), 3, CV_AA );
    }


    imshow( "Result", image );

    if(waitKey(1) >= 0) break;

}

return 0;
}
