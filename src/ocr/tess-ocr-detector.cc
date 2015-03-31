#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <libconfig.h>


// tesseract
#include <tesseract/baseapi.h>
#include <tesseract/resultiterator.h>

// accurate frame reading
#include "repere.h"
#include "binarize.h"
#include "xml.h"

#include <algorithm> 
#include <functional> 
#include <cctype>
#include <locale>



namespace amu {

    class Result {
        public:
            std::string text;
            double confidence;
            Result() : confidence(0) { }
    };

    class OCR {
        int imageWidth, imageHeight;
        tesseract::TessBaseAPI tess; 
        public:
        OCR(const std::string& datapath="", const std::string& lang="fra") : imageWidth(0), imageHeight(0) {
            if(datapath != "") setenv("TESSDATA_PREFIX", std::string(datapath + "/../").c_str(), 1);
            tess.Init(NULL,lang.c_str(), tesseract::OEM_DEFAULT);
            SetMixedCase();
        }

        void SetUpperCase(bool ignoreAccents = false) {
            if(ignoreAccents) 
                tess.SetVariable("tessedit_char_whitelist", "!\"$%&'()+,-./0123456789:;?@ABCDEFGHIJKLMNOPQRSTUVWXYZ\\_ «°»€");
            else 
                tess.SetVariable("tessedit_char_whitelist", "!\"$%&'()+,-./0123456789:;?@ABCDEFGHIJKLMNOPQRSTUVWXYZ\\_ «°»ÀÂÇÈÉÊËÎÏÔÙÚÛÜ€");
        }

        void SetMixedCase(bool ignoreAccents = false) {
            if(ignoreAccents) 
                tess.SetVariable("tessedit_char_whitelist", "!\"$%&'()+,-./0123456789:;?@ABCDEFGHIJKLMNOPQRSTUVWXYZ\\_abcdefghijklmnopqrstuvwxyz «°»€");
            else 
                tess.SetVariable("tessedit_char_whitelist", "!\"$%&'()+,-./0123456789:;?@ABCDEFGHIJKLMNOPQRSTUVWXYZ\\_abcdefghijklmnopqrstuvwxyz «°»ÀÂÇÈÉÊËÎÏÔÙÚÛÜàâçèéêëîïôöùû€");
        }

        ~OCR() {
        }

        void SetImage(cv::Mat& image) {
            tess.SetImage((uchar*) image.data, image.size().width, image.size().height, image.channels(), image.step1());
            imageWidth = image.size().width;
            imageHeight = image.size().height;
        }

        Result Process(cv::Mat& image) {
            SetImage(image);
            return Process();
        }

        static std::string ProtectNewLines(const std::string& text) {
            std::string str = text;
            std::string oldStr = "\n";
            std::string newStr = " ";
            size_t pos = 0;
            while((pos = str.find(oldStr, pos)) != std::string::npos)
            {
                str.replace(pos, oldStr.length(), newStr);
                pos += newStr.length();
            }
            return str;
        }

        Result Process() {
            Result result;
            tess.Recognize(NULL);
            const char* text = tess.GetUTF8Text();
            result.text = ProtectNewLines(std::string(text));
            amu::Trim(result.text);
            delete text;
            result.confidence = tess.MeanTextConf() / 100.0;
            return result;
        }

    };

}


cv::Mat sobel_filter_H(cv::Mat image){                                   
    cv::Mat  image_sobel_h;
    cv::Sobel(image, image_sobel_h,CV_16S, 1, 0, 1, 1, 0, cv::BORDER_DEFAULT );                                 
    cv::convertScaleAbs(image_sobel_h,image_sobel_h,1,0);
    cv::threshold(image_sobel_h, image_sobel_h, 103, 255, cv::THRESH_BINARY);    
    return  image_sobel_h;
}



// dilatation and erosion
cv::Mat dilataion_erosion(cv::Mat image){     

	cv::Mat element = cv::getStructuringElement( cv::MORPH_RECT, 
					  cv::Size(3, 1), 
					  cv::Point(1,0));
					  
	cv::dilate(image, image, element, cv::Point(1,0) , 19); 
	cv::erode(image, image, element, cv::Point(1,0) , 19); 
    return image;
}




cv::Mat delete_horizontal_bar(cv::Mat image){
	
	
	double y_min_size_text=	3;
	cv::Size s = image.size();
	cv::rectangle(image,cv::Point(0,0),cv::Point(s.width-1,s.height-1),cv::Scalar(0,0,0,0),2,4,0) ;    
	cv::Mat element1 = cv::getStructuringElement( cv::MORPH_RECT,cv::Size( 1, 2 ),cv::Point( 0, 1 ) );                                      
	cv::Mat element2 = cv::getStructuringElement( cv::MORPH_RECT,cv::Size( 1, 2 ),cv::Point( 0, 0 ) );                                      
	cv::erode(image, image, element1, cv::Point(0,1) , y_min_size_text) ;
	cv::dilate(image, image, element2, cv::Point(0,0) , y_min_size_text);  
    return image;
}




cv::Mat delete_vertical_bar(cv::Mat image){
	cv::Size s = image.size();
	double x_min_size_text = 34;

	cv::rectangle(image,cv::Point(0,0),cv::Point(s.width-1,s.height-1),cv::Scalar(0,0,0,0),2,4,0) ;    
	cv::Mat element1 = cv::getStructuringElement( CV_SHAPE_RECT,cv::Size( 2, 1 ),cv::Point( 1, 0 ) );                                      
	cv::Mat element2 = cv::getStructuringElement( CV_SHAPE_RECT,cv::Size( 2, 1 ),cv::Point( 0, 0 ) );                                      
	cv::erode(image, image, element1, cv::Point(1,0) , x_min_size_text) ;
	cv::dilate(image, image, element2, cv::Point(0,0) , x_min_size_text);  
    return image;
}


//find the nearest integer of a float   
int round_me(float x) {                                   
     if ((int)(x+0.5)==(int)(x)) return (int)(x);
     else return (int)(x+1);
}



bool cmp_points(cv::Point pt1, cv::Point pt2) { 
		    int y_diff = pt1.y - pt2.y;
			int x_diff = pt1.x - pt2.x;
			return (y_diff ? y_diff : x_diff);
		}

bool  cmp_points2(cv::Point pt1, cv::Point pt2){
	if(pt1.y!=pt2.y)
		return (pt1.y<pt2.y);
	else
		return (pt1.x<pt2.x);
}


// Find global threshold with Sauvola binarisation algorithm
int sauvola(cv::Mat im_thr, int video_depth=256){
    float k_sauvola=0.5;
	cv::Size s = im_thr.size();
	bitwise_not(im_thr,im_thr);	
	
	float mean_sauv=0.0;
	float total_px_sauv=0;
	int nb_px_sauv=0;
    int val=0;
    float connect[video_depth];
    int x,y,i;
    for(i=0;i<video_depth;i++){
    	connect[i]=0;
    }
    
	for (x=0;x<s.width;x++){
		for (y=0;y<s.height;y++){
			val=im_thr.at<unsigned char>(y, x);
			if (val!=0) connect[val]++;
			else connect[val]++;
		}
	}
	
	for (x=0;x<s.width;x++){
		for (y=0;y<s.height;y++){
			val=im_thr.at<unsigned char>(y, x);
			total_px_sauv+=val;
			nb_px_sauv++;
		}
	}	
	mean_sauv=total_px_sauv/nb_px_sauv;
	
	float ecart_type_sauv=0.0;
	for(i=0;i<video_depth-1;i++) ecart_type_sauv+=(connect[i]*(i-mean_sauv)*(i-mean_sauv));
	
	ecart_type_sauv/=total_px_sauv;
	ecart_type_sauv=sqrt(ecart_type_sauv);
	float thr_sauv=mean_sauv*(1+k_sauvola*(ecart_type_sauv/(video_depth/2)-1));  // Tsauvola = m (1 + k(s/R -1))	
	return 255-round_me(thr_sauv);
}



bool in_rects(cv::Rect rect, std::vector<cv::Rect> rects, int * idx)
{
	int x,y, width, height;
	cv::Rect intersection;
	float area_1, area_2, area_3;
	area_1= rect.width*rect.height;

	for (int i=0; i<rects.size(); i++){
		x = rects[i].x;
		y = rects[i].y;
		width = rects[i].width;
		height = rects[i].height;
		
		intersection = rect & rects[i];
		area_2= width*height;
		area_3= intersection.width*intersection.height;
		float recouvrement=area_3/(area_1 + area_2 - area_3);
		if (recouvrement>0.5) {
			*idx=i;
			return true;
			
			};
		}
	
	return false;
	
	}


void show_contours(cv::Mat image , std::vector<std::vector<cv::Point> > contours, std::vector<cv::Vec4i> hierarchy){
	cv::Mat drawing = cv::Mat::zeros(image.size(), CV_8UC3 );
	for( int i = 0; i< contours.size(); i++ ){
       cv::drawContours( drawing, contours, i, (255,255,255), 2, 8, hierarchy, 0, cv::Point() );
     }
	cv::imshow( "Contours", drawing );
	cv::waitKey(0);
}


// coarse spatial detection of boxes
std::vector<cv::Rect> find_boxes(cv::Mat im,  cv::Mat frame_BW, cv::Mat im_mask, bool isMask){ 
	std::vector<cv::Rect> rects;	
	
	double y_min_size_text=	3;
	double x_min_size_text = 34;
	float ratio_width_height = 3.24;

    int i, j;
    std::vector<std::vector<cv::Point> > contours;
    std::vector<cv::Vec4i> hierarchy;
    
	cv::Size s = im.size();
	cv::rectangle(im,cv::Point(0,0),cv::Point(s.width-1,s.height-1),cv::Scalar(0,0,0,0),2,4,0) ;    
    
			    
	// contours detection
	cv::findContours(im, contours, hierarchy, CV_RETR_LIST, CV_LINK_RUNS, cv::Point(0, 0) );
	
	//show contours
    //show_contours(im , contours, hierarchy);

    
    //coarse detection
	for( int n = 0; n< contours.size(); n++ ){
		//corners coordinates initialization
		int y_min=s.height-2;
		int x_min=s.width-2;
		int y_max=2;
		int x_max=2;
		
		
		std::vector<cv::Point> current_contour=contours[n];
		
		// sort coordinates in order to make each pair of consecutive points a line in the contour
		std::stable_sort(current_contour.begin(), current_contour.end(), cmp_points2);
		
		for(i=0; i<current_contour.size(); i=i+2 ) { 
            cv::Point p1 = current_contour[i];
            cv::Point p2 = current_contour[i+1];   
			if ((p2.x - p1.x) >= x_min_size_text){ // find corners if width's line is greater than the minimum
                if (p1.y < y_min) y_min = p1.y;   // top left corner
                if (p1.x < x_min) x_min = p1.x;   // top left corner
                if (p2.y > y_max) y_max = p2.y;   // bottom right corner
                if (p2.x > x_max) x_max = p2.x;   // bottom right corner
            }
        } 
        
        if (y_min<2) y_min=2;
        if (x_min<2) x_min=2;
        if (y_max>s.height-2) y_max=s.height-2;
        if (x_max>s.width-2) x_max=s.width-2;
        
        if (y_max-y_min>=y_min_size_text && (float)(x_max-x_min)>=ratio_width_height*(float)(y_max-y_min)){ //if the box has good size 
            int y_min_final=y_min;
            int x_min_final=x_min;
            int y_max_final=y_max;
            int x_max_final=x_max;

			cv::Rect roi(x_min-2, y_min-2, x_max-x_min+4, y_max-y_min+4);
			cv::Mat im_temp = frame_BW(roi);
			int threshold_found = -1;                      
			threshold_found = sauvola(im_temp);          	
               
            if (threshold_found!=0){   
            	y_min_final=y_min_final-2;  if (y_min_final<0) y_min_final=0;  
                x_min_final=x_min_final-2;  if (x_min_final<0) x_min_final=0;
                y_max_final=y_max_final+4;  if (y_max_final>=s.height) y_max_final=s.height;
                x_max_final=x_max_final+4;  if (x_max_final>=s.width) x_max_final=s.width;
    
                cv::Rect roi2(x_min_final, y_min_final, x_max_final-x_min_final, y_max_final-y_min_final);
				cv::Mat im_box = frame_BW(roi2);
                cv::Size s2=im_box.size();
			
				cv::Rect rect ;
			    rect.x =x_min_final;
			    rect.y =y_min_final;
			    rect.width = x_max_final -x_min_final ;
			    rect.height = y_max_final -y_min_final;
				int idx;
				if (rects.size()==0 || (!in_rects(rect, rects, &idx))) rects.push_back(rect);
				else rects[idx] = rects[idx] | rect; // if rect is in rects than update the rectangle as the union
                
            }		 
			 
		}
	}

return rects;
}



















int main(int argc, char** argv) {

    amu::CommandLine options(argv, "[options]\n");
    options.AddUsage("  --data <directory>                tesseract model directory (containing tessdata/)\n");
    options.AddUsage("  --lang <language>                 tesseract model language (default fra)\n");
    options.AddUsage("  --mask                            use mask for text box search\n");
    options.AddUsage("  --upper-case                      contains only upper case characters\n");
    options.AddUsage("  --ignore-accents                  do not predict letters with diacritics\n");
    options.AddUsage("  --sharpen                         sharpen image before processing\n");
    options.AddUsage("  --show                            show image beeing processed\n");
    options.AddUsage("  --step                            specify processing every step frame (default 1)\n");


    double zoom = options.Read("--scale", 1.0); 
    std::string dataPath = options.Get<std::string>("--data", "");
    std::string lang = options.Get<std::string>("--lang", "fra");
    bool upper_case = options.IsSet("--upper-case");
    bool ignore_accents = options.IsSet("--ignore-accents");
    bool sharpen = options.IsSet("--sharpen");
    bool show = options.IsSet("--show");
    std::string maskFile = options.Get<std::string>("--mask", "");
    
    
    
     // read configuration file 
    config_t cfg;
    config_setting_t *s;
    config_init(&cfg);
    int step=1;
    
    if (config_read_file(&cfg, "configure/configure.cfg") == CONFIG_TRUE) {
      s = config_lookup(&cfg, "ocr.step");
      step = config_setting_get_int(s);
	}
	config_destroy(&cfg);
    
    step = options.Read("--step", step);

	cv::Mat mask;
	bool isMask=false;
	
	if (maskFile!=""){
		isMask=true;
		mask = cv::imread(maskFile, CV_LOAD_IMAGE_COLOR); 
		cv::resize(mask,mask,cv::Size(1024,576));
	}
				
		
		
	std::cout << "<?xml version=\"1.0\" ?>\n";
	std::cout << "<boxes>\n";
	amu::OCR ocr(dataPath, lang);		    
    if(upper_case) ocr.SetUpperCase(ignore_accents);
    else ocr.SetMixedCase(ignore_accents);

    amu::VideoReader video;
    if(!video.Configure(options)) return 1;
    if(options.Size() != 0) options.Usage();
    
	cv::Mat image, image_BW, resized;
	while(video.HasNext()) {
        if(video.ReadFrame(image)) {		
				cv::resize(image,resized,cv::Size(1024,576));
				// apply mask if it exists
				if (isMask) cv::bitwise_and(resized, mask, resized);	
				
				cv::cvtColor(resized, image_BW, CV_RGB2GRAY);		
				cv::Mat im=image_BW;
				im=sobel_filter_H(image_BW);			
				im = dilataion_erosion(im);
				im = delete_horizontal_bar(im);
				im = delete_vertical_bar(im);
				std::vector<cv::Rect> rects = find_boxes(im, image_BW, mask, isMask);
				      
				float time =video.GetTime();
				for (int i=0; i<rects.size();i++) {
					
					amu::Result result;
					result.confidence = 0;
					result.text = "TESSERACT_FAILED";
					cv::Rect roi(rects[i].x, rects[i].y, rects[i].width, rects[i].height);
					cv::Mat im_box = resized(roi);
					ocr.SetImage(im_box);
					result = ocr.Process();
					
					std::cout << "<box>\n";
					std::cout <<"  <time> "<<video.GetTime()<< " </time>\n";
					std::cout <<"  <position_X> "<<rects[i].x<< " </position_X>\n";
					std::cout <<"  <position_Y> "<<rects[i].y<< " </position_Y>\n";
					std::cout <<"  <width> "<<rects[i].width<< " </width>\n";
					std::cout <<"  <height> "<<rects[i].height<< " </height>\n";
					std::cout <<"  <confidence> "<<result.confidence  << " </confidence>\n";
					std::cout <<"  <text> " <<result.text << " </text>\n";
					std::cout << "</box>\n";
					
					if(show) {
						cv::imshow("original", im_box);
						cv::waitKey(600);
						cv::destroyWindow("original");
					}
				}
			
		}
	if (step >1) video.SeekTime(video.GetTime()+0.04*(step-1));
	}
	std::cout << "</boxes>\n";
	
    return 0;
}
