// Face detection in videos based on haar descriptors
// Please read the wiki for information and build instructions.

#include <iostream>
#include <opencv2/opencv.hpp>
#include "video.h"
#include "commandline.h"
#include <libconfig.h>

double skin(cv::Mat &image) {
    cv::Mat hsv;
    cv::cvtColor(image, hsv, CV_BGR2HSV);
    cv::Mat hue(image.size(), CV_8UC1), saturation(image.size(), CV_8UC1), value(image.size(), CV_8UC1);
    std::vector<cv::Mat> channels;
    channels.push_back(hue);
    channels.push_back(saturation);
    channels.push_back(value);
    cv::split(hsv, channels);
    cv::threshold(hue, hue, 18, 255, CV_THRESH_BINARY_INV);
    cv::threshold(saturation, saturation, 50, 255, CV_THRESH_BINARY);
    cv::threshold(value, value, 80, 255, CV_THRESH_BINARY);
    cv::Mat output;
    cv::bitwise_and(hue, saturation, output);
    cv::bitwise_and(output, value, output);
    cv::Scalar sums = cv::sum(output);
    double result = sums[0] / (output.rows * output.cols * 255);
    return result;
}

int main(int argc, char** argv) {
    
      std::cout<<"*********** Face Detection ***********"<<std::endl;	
	// Usages
    amu::CommandLine options(argv, "[options]\n");
    options.AddUsage("  --model                           \n");
    options.AddUsage("  --show                            show results\n");
    options.AddUsage("  --step                            ****\n");
    
		
	bool show = options.IsSet("--show");
    std::string model= options.Get<std::string>("--model", "haarcascade_frontalface_alt2.xml");
    cv::CascadeClassifier detector(model);
    if(detector.empty()) {
        std::cerr << "ERROR: could not find model \"" << model << "\"\n";
        return 1;
    }
    
    // load video
    amu::VideoReader video;
    if(!video.Configure(options)) return 1;
    if(options.Size() != 0) options.Usage();
    cv::Mat image, copy, gray;
    std::vector<cv::Rect> detections;

                
	while(video.HasNext()) {
        if(video.ReadFrame(image)) {		
			if(show) image.copyTo(copy);    
            detections.clear();

            detector.detectMultiScale(image, detections);
            std::vector<cv::Rect>::iterator detection = detections.begin();
            for(size_t i = 0; i < detections.size(); i++) {
                cv::Mat face(image, detection[i]);
                std::cout << video.GetIndex() << " "<<video.GetTime() <<" "<<detections[i].x << " " << detections[i].y
                                << " " << detections[i].width << " " << detections[i].height << " " << skin(face) << std::endl;
                if (show) cv::rectangle(image, detections[i], cv::Scalar(255, 0, 0), 1);
            }
            
                                
        if (show) {    
		cv::imshow( "Display window", image );              
		if(cv::waitKey(1) == 27) break;
        }
        
	}
    
}
}
