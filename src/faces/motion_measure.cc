// motion measure in videos 
// Please read the wiki for information and build instructions.

#include <iostream>
#include <opencv2/opencv.hpp>
#include "video.h"
#include "commandline.h"
#include <libconfig.h>

#include <libxml/parser.h>
#include <libxml/tree.h>
using namespace cv;
using namespace std; 


double motion(cv::Mat imgA,cv::Mat imgB ) {
	double mv=0;
    flow = cv2.calcOpticalFlowFarneback(boite1, boite2, 0.5, 3, 1, 3, 5, 1.2, 0)
    
    std::vector<uchar> features_found;
    features_found.reserve(maxCorners);
 	std::vector<float> feature_errors; 
	feature_errors.reserve(maxCorners);
   
	calcOpticalFlowPyrLK( imgA, imgB, cornersA, cornersB, features_found, feature_errors ,
		Size( win_size, win_size ), 5,
		 cvTermCriteria( CV_TERMCRIT_ITER | CV_TERMCRIT_EPS, 20, 0.3 ), 0 );


	return mv;
}




int main(int argc, char** argv) {
    std::cerr<<"*********** face motion ***********"<<std::endl;	
	// Usages
    amu::CommandLine options(argv, "[options]\n");

 // load video
    amu::VideoReader video;
    if(!video.Configure(options)) {
        std::cerr << "Requires video\n";
        return 1;
    }
    cv::Mat image, image_previous;
    video.ReadFrame(image);
    image_previous=image;
    int frame=video.GetIndex();
    int frame_previous=video.GetIndex();
	while(video.HasNext()) {
        if(video.ReadFrame(image)) {
			    frame=video.GetIndex();
			    std::cout <<frame <<" " << frame_previous <<std::endl;
				cv::imshow( "Display window", image );  
				if(cv::waitKey(1) == 27) break;
				image_previous=image;
				frame_previous=frame;
		}
	}
	
	return 0;
}
