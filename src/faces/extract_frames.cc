#include <iostream>
#include <list>
#include <string>
#include "repere.h"
#include "commandline.h"
#include "face.h"

int main(int argc, char** argv) {
    std::cout<<"*********** Extract shot frames ***********"<<std::endl;	
    amu::CommandLine options(argv, "[options]\n");
    options.AddUsage("  --shots <shot-file>               shot segmentation output\n");
    options.AddUsage("  --idx <idx-file>                  idx for converting frames to times\n");

    std::string shotFilename = options.Get<std::string>("--shots", "");
    std::string idxFilename = options.Get<std::string>("--idx", "");
    std::string filename(shotFilename);
    std::string f;
    if(options.Size() == 0)  options.Usage();

    filename.replace( filename.end()-6, filename.end(),"");
    amu::VideoReader video;
    if(!video.Configure(options)) {
        return 1;
    }
    
    std::vector<amu::ShotSegment> shots;
    shots = amu::ShotSegment::Read(shotFilename);
	cv::Mat image;
    for(size_t shot =0; shot < shots.size(); shot++) {
        int frame = shots[shot].frame;
        video.Seek(frame);
		//std::cout <<shots[shot].startFrame <<" "<< shots[shot].endFrame  << " " << frame <<std::endl;
		video.ReadFrame(image);
		std::ostringstream ss;
		ss << frame;
    	f=filename + "_" + ss.str() +".jpg";
		//std::cout <<f <<std::endl;
		cv::imwrite( f, image );
		//cv::namedWindow( "Gray image", CV_WINDOW_AUTOSIZE );
		//cv::imshow("Gray image", image );
		//cv::waitKey(0);
	}

	
	return 0;
}
