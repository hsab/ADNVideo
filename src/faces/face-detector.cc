// Face detection in videos based on haar descriptors
// Please read the wiki for information and build instructions.

#include <iostream>
#include <opencv2/opencv.hpp>
#include "video.h"
#include "commandline.h"
#include <libconfig.h>

#include <libxml/parser.h>
#include <libxml/tree.h>

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
    options.AddUsage("  --model                           haarcascade model (XML format) \n");
    options.AddUsage("  --step                            run detection every <step> frames (default 1)\n");
    options.AddUsage("  --threshold                       filter faces following skin color (default 0.0)\n");
    options.AddUsage("  --show                            display face detection \n");
    std::string output = options.Get<std::string>("--output", "face_results.xml");

	bool show = options.IsSet("--show");
    std::string model= options.Get<std::string>("--model", "/usr/share/opencv/haarcascades/haarcascade_frontalface_alt2.xml");
    
	// read configuration file 
    config_t cfg;
    config_setting_t *s;
    config_init(&cfg);
    int step=1; 
	double threshold=0.0;
    if (config_read_file(&cfg, "configure/configure.cfg") == CONFIG_TRUE) {
		s = config_lookup(&cfg, "face.step");
		step = config_setting_get_int(s);
		s = config_lookup(&cfg, "face.threshold");
		threshold = config_setting_get_int(s);
	}
	config_destroy(&cfg);    
    step = options.Read("--step", step);
    threshold = options.Read("--threshold", threshold);

    // load video
    amu::VideoReader video;
    if(!video.Configure(options)) {
        std::cerr << "Requires video\n";
        return 1;
    }

    
    cv::CascadeClassifier detector(model);
    if(detector.empty()) {
        std::cerr << "ERROR: could not find model \"" << model << "\"\n";
        return 1;
    }
    //XML output
	xmlDocPtr doc = NULL;      
    xmlNodePtr root_node = NULL, node = NULL, box_node = NULL;
    doc = xmlNewDoc(BAD_CAST "1.0");
    root_node = xmlNewNode(NULL, BAD_CAST "root");
    xmlDocSetRootElement(doc, root_node);
    

    cv::Mat image, copy, gray;
    std::vector<cv::Rect> detections;                
    char b[100];
    double score;
	while(video.HasNext()) {
        if(video.ReadFrame(image)) {		
			if(show) image.copyTo(copy);    
            detections.clear();

            detector.detectMultiScale(image, detections);
            std::vector<cv::Rect>::iterator detection = detections.begin();
            for(size_t i = 0; i < detections.size(); i++) {
                cv::Mat face(image, detection[i]);
                score = skin(face);
                if (show & score>threshold) {
					std::cout << video.GetIndex() << " "<<video.GetTime() <<" "<<detections[i].x << " " << detections[i].y << " " << detections[i].width << " " << detections[i].height << " " << score << std::endl;
					cv::rectangle(image, detections[i], cv::Scalar(255, 0, 0), 2);
					sprintf(b, "%.2f",skin(face));
                    cv::putText(image, b, cv::Point(detections[i].x - 15, detections[i].y - 15), cv::FONT_HERSHEY_COMPLEX_SMALL, 1, cv::Scalar(0, 0, 255));
                    
				}

				box_node=xmlNewChild(root_node, NULL, BAD_CAST "face", BAD_CAST NULL);
				char buffer[100];
				sprintf(buffer, "%f",video.GetTime());
				node = xmlNewChild(box_node, NULL, BAD_CAST "time", (const xmlChar *) buffer);
				
				sprintf(buffer, "%d",detections[i].x);
				node = xmlNewChild(box_node, NULL, BAD_CAST "positionX", (const xmlChar *) buffer);
				
				sprintf(buffer, "%d",detections[i].y);	
				node = xmlNewChild(box_node, NULL, BAD_CAST "positionY", (const xmlChar *) buffer);
				
				sprintf(buffer, "%d",detections[i].width);
				node = xmlNewChild(box_node, NULL, BAD_CAST "width", (const xmlChar *) buffer);
				
				sprintf(buffer, "%d",detections[i].height);
				node = xmlNewChild(box_node, NULL, BAD_CAST "height", (const xmlChar *) buffer);
					
				sprintf(buffer, "%f",score );
				node = xmlNewChild(box_node, NULL, BAD_CAST "confidence",(const xmlChar *) buffer);
					
                
                
            }                      
			if (show) {    
				cv::imshow( "Display window", image );  
				if(cv::waitKey(1) == 27) break;
			}
  
		}
    		//	seek to time + step
		if (step >1) video.Seek(video.GetIndex()+step);  
		
	}
	
	
	char *o_file = new char[output.length() + 1];
	strcpy(o_file, output.c_str());					
	xmlSaveFormatFileEnc(o_file, doc, "UTF-8", 1);
	delete [] o_file ;	
					
					
	xmlFreeDoc(doc);
    xmlCleanupParser();
    xmlMemoryDump();
    return 0;
}
