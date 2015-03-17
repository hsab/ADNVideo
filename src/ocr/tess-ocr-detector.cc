#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>


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

    class Rect {
        public:
            double start, end;
            int x, y, width, height;
            std::vector<std::string> text;

            Rect() : start(0), end(0), x(0), y(0), width(0), height(0) { }

            Rect(const Rect& other, double factor = 1) : start(other.start), end(other.end), x(other.x * factor), y(other.y * factor), width(other.width * factor), height(other.height * factor), text(other.text) { }

            Rect(int _x, int _y, int _width, int _height) : start(0), end(0), x(_x), y(_y), width(_width), height(_height) { }

            Rect(double _start, double _end, int _x, int _y, int _width, int _height, std::vector<std::string> _text = std::vector<std::string>()) : start(_start), end(_end), x(_x), y(_y), width(_width), height(_height), text(_text) { }

            operator cv::Rect() {
                return cv::Rect(x, y, width, height);
            }

            static double TimeFromString(const std::string &text) {
                // PT1M57.040S
                if(text.length() < 3 || text[0] != 'P' || text[1] != 'T' || text[text.length() - 1] != 'S') {
                    std::cerr << "ERROR: cannot parse time \"" << text << "\"\n";
                    return 0;
                }
                int hLocation = text.find('H');
                int mLocation = text.find('M');
                double hours = 0, minutes = 0, seconds = 0;
                if(hLocation != -1) {
                    hours = strtod(text.c_str() + 2, NULL);
                    if(mLocation != -1) {
                        minutes = strtod(text.c_str() + hLocation + 1, NULL);
                        seconds = strtod(text.c_str() + mLocation + 1, NULL);
                    } else {
                        std::cerr << "ERROR: cannot parse time \"" << text << "\"\n";
                    }
                } else {
                    if(mLocation != -1) {
                        minutes = strtod(text.c_str() + 2, NULL);
                        seconds = strtod(text.c_str() + mLocation + 1, NULL);
                    } else {
                        seconds = strtod(text.c_str() + 2, NULL);
                    }
                }
                return 3600 * hours + 60 * minutes + seconds;
            }

            static std::string StringFromTime(double time) {
                int hours = ((int) time) / 3600;
                int minutes = (((int) time) / 60) % 60;
                double seconds = time - hours * 3600 - minutes * 60;
                std::stringstream output;
                output << "PT";
                if(hours != 0) output << hours << "H";
                if(minutes != 0) output << minutes << "M";
                char* remainder = NULL;
                asprintf(&remainder, "%.03fS", seconds);
                output << remainder;
                free(remainder);
                return output.str();
            }


            static std::vector<amu::Rect> ReadAll(std::istream& input, std::vector<std::string>& tail, int x_offset = 4, int y_offset = -1) {
                std::vector<amu::Rect> rects;
                std::vector<std::string> text;
                double time = -1;
                double duration = -1;
                int x = -1, y = -1, width = -1, height = -1;
                int line_num = 0;
                while(!input.eof()) {
                    std::string line;
                    if(!std::getline(input, line)) break;
                    line_num ++;
                    if(amu::StartsWith(line, "<Eid ")) {
                        std::string positionString = line.substr(std::string("<Eid name=\"TextDetection\" taskName=\"TextDetectionTask\" position=\"").length());
                        size_t timeEnd = positionString.find('"'); 
                        std::string timeString = positionString.substr(0, timeEnd);
                        time = TimeFromString(timeString);
                        size_t positionStart = timeEnd + std::string("\" position=\"").length();
                        size_t positionEnd = positionString.find('"', positionStart + 1);
                        std::string durationString = positionString.substr(positionStart, positionEnd - positionStart);
                        duration = TimeFromString(durationString);
                    } else if(amu::StartsWith(line, "<IntegerInfo name=\"Position_X\">")) {
                        x = strtol(line.substr(std::string("<IntegerInfo name=\"Position_X\">").length()).c_str(), NULL, 10);
                    } else if(amu::StartsWith(line, "<IntegerInfo name=\"Position_Y\">")) {
                        y = strtol(line.substr(std::string("<IntegerInfo name=\"Position_Y\">").length()).c_str(), NULL, 10);
                    } else if(amu::StartsWith(line, "<IntegerInfo name=\"Width\">")) {
                        width = strtol(line.substr(std::string("<IntegerInfo name=\"Width\">").length()).c_str(), NULL, 10);
                    } else if(amu::StartsWith(line, "<IntegerInfo name=\"Height\">")) {
                        height = strtol(line.substr(std::string("<IntegerInfo name=\"Height\">").length()).c_str(), NULL, 10);
                    }
                    text.push_back(line);
                    if(line == "</Eid>") {
                        if(time == -1 || duration == -1 || x == -1 || y == -1 || width == -1 || height == -1) {
                            std::cerr << "WARNING: incomplete rectangle ending at line " << line_num << "\n";
                        }
                        rects.push_back(amu::Rect(time, time + duration, x + x_offset, y + y_offset, width - x_offset, height - y_offset, text));
                        x = y = width = height = -1;
                        time = -1;
                        duration = -1;
                        text.clear();
                    }
                }
                tail = text;
                return rects;
            }
    };

    struct RectLess {
        bool operator ()(Rect const& a, Rect const& b) const {
            if(a.start < b.start) return true;
            return false;
        }
    };

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
            if(datapath != "") {
                setenv("TESSDATA_PREFIX", std::string(datapath + "/../").c_str(), 1);
            }
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

        void SetRectangle(const Rect& rect) {
            tess.SetRectangle(rect.x, rect.y, rect.width, rect.height);
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
    cv::Sobel(image, image_sobel_h, CV_16S, 1, 0, 3, 1, 0, cv::BORDER_DEFAULT );                                 
    cv::convertScaleAbs(image_sobel_h,image_sobel_h,1,0);
    cv::threshold(image_sobel_h, image_sobel_h, 103, 256, CV_THRESH_BINARY);       //verifier
    return  image_sobel_h;
}


// dilatation and erosion
cv::Mat dilataion_erosion(cv::Mat image){     
    cv::Mat element = cv::getStructuringElement( CV_SHAPE_RECT,cv::Size( 3, 1 ),cv::Point( 1, 0 ) );                                      
    cv::dilate(image, image, element);
    cv::erode(image, image, element);
    
    return image;
}


cv::Mat delete_horizontal_bar(cv::Mat image){
	//cv::Size s = image.size();
	//cv::rectangle(image,cv::Point(0,0),cv::Point(s.width-1,s.height-1),cv::Scalar(0,0,0,0),2,4,0) ;    
	cv::Mat element1 = cv::getStructuringElement( CV_SHAPE_RECT,cv::Size( 1, 2 ),cv::Point( 0, 1 ) );                                      
	cv::Mat element2 = cv::getStructuringElement( CV_SHAPE_RECT,cv::Size( 1, 2 ),cv::Point( 0, 0 ) );                                      
	cv::erode(image, image, element1) ;
	cv::dilate(image, image, element2);  
    return image;
}


cv::Mat delete_vertical_bar(cv::Mat image){
	//cv::Size s = image.size();
	//cv::rectangle(image,cv::Point(0,0),cv::Point(s.width-1,s.height-1),cv::Scalar(0,0,0,0),2,4,0) ;    
	cv::Mat element1 = cv::getStructuringElement( CV_SHAPE_RECT,cv::Size( 2, 1 ),cv::Point( 1, 0 ) );                                      
	cv::Mat element2 = cv::getStructuringElement( CV_SHAPE_RECT,cv::Size( 2, 1 ),cv::Point( 0, 0 ) );                                      
	cv::erode(image, image, element1) ;
	cv::dilate(image, image, element2);  
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


// Find global threshold with Sauvola binarisation algorithm
int sauvola(cv::Mat im_thr, int video_depth=1){
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
			connect[val]++;
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
	bitwise_not(im_thr,im_thr);
	return 255-round_me(thr_sauv);
}




// Find threshold to delete noise in the image
double noise_image(cv::Mat im, int thr_otsu ){
   int var=0;
 
    return var;    
}





// coarse spatial detection of boxes
std::vector<amu::Rect> find_boxes(cv::Mat image, int frameNum, cv::Mat frame_BW_original, cv::Mat frame_4, cv::Mat frame_5, cv::Mat im_mask, bool isMask){ 
	std::vector<amu::Rect> rects;	
	
	double y_min_size_text=	0.0056;
	double x_min_size_text = 0.0466;
	double coef_increase_thr_otsu = 1.42;
	double ratio_width_height = 2.275;
	
    int i, j, idx;
    std::vector<std::vector<cv::Point> > contours;
    std::vector<cv::Vec4i> hierarchy;
    
	cv::Size s = image.size();
	cv::rectangle(image,cv::Point(0,0),cv::Point(s.width-1,s.height-1),cv::Scalar(0,0,0,0),2,4,0) ;    
	
	// contours detection
	cv::findContours(image, contours, hierarchy, CV_RETR_LIST, CV_LINK_RUNS, cv::Point(0, 0) );
	
	cv::Mat drawing = cv::Mat::zeros( image.size(), CV_8UC3 );
	for( int i = 0; i< contours.size(); i++ ){
       cv::drawContours( drawing, contours, i, (255,255,255), 2, 8, hierarchy, 0, cv::Point() );
     }

	cv::imshow( "Contours", drawing );
	cv::waitKey(0);

	
    
    
    //coarse detection
    int y_min=s.height-2;
    int x_min=s.width-2;
    int y_max=2;
    int x_max=2;
    
	for( int n = 0; n< contours.size(); n++ ){
		
    std::vector<cv::Point> current_contour=contours[n];
	
	std::stable_sort(current_contour.begin(), current_contour.end(), cmp_points);
		
	for(i=0; i<current_contour.size(); i=i+2 ) {         // each pair of point corresponds to a line in the contour
            cv::Point p1 = current_contour[i];
            cv::Point p2 = current_contour[i+1];   
           if ((p2.x - p1.x) >= 1){              // if the line has a width greater than the minimum width
                if (p1.y < y_min) y_min = p1.y;  // find the top left corner
                if (p1.x < x_min) x_min = p1.x;  // find the top left corner
                if (p2.y > y_max) y_max = p2.y;  // find the bottom right corner
                if (p2.x > x_max) x_max = p2.x;  // find the bottom right corner
            }
        } 
        
        
        x_min = x_min + 2 + 1;                        // offset due to previous operation                                    
        x_max = x_max + 2 + 1;  
        y_min = y_min + 1;                                               
        y_max = y_max + 1;
        
        
        if (y_min<2) y_min=2;
        if (x_min<2) x_min=2;
        if (y_max>s.height-2) y_max=s.height-2;
        if (x_max>s.width-2) x_max=s.width-2;
        
        	 	 	 	 
         if (y_max-y_min>=y_min_size_text && (float)(x_max-x_min)>=ratio_width_height*(float)(y_max-y_min)){         //if the box has the good geometry
			 
            int y_min_final=y_min;
            int x_min_final=x_min;
            int y_max_final=y_max;
            int x_max_final=x_max;

            cv::Mat frame_BW = frame_BW_original;

            //text color detection (black on white or vice versa)
            cv::Rect roi(x_min-2, y_min-2, x_max-x_min+4, y_max-y_min+4);
            cv::Mat im_temp = frame_BW(roi);
  
                   
		
            cv::Mat im_temp_mask ;
            if (isMask) {                
                im_temp_mask = im_mask(roi);
            }             
            
           int threshold_found = -1;            
            int nb_cc=0;
            //if (param->type_threshold==-1) {
            //     threshold_found = cvThreshold(im_temp, im_temp, param->max_thr, param->max_thr, CV_THRESH_TOZERO+CV_THRESH_OTSU);
             //    threshold_found = param->coef_increase_thr_otsu*threshold_found;
            //}
            //else if (param->type_threshold==-2) threshold_found = sauvola(im_temp, param->max_thr+1);
            //else if (param->type_threshold==-3) threshold_found = wolf(im_temp, param->max_thr+1);
            //else threshold_found=param->type_threshold;
            
               
            //if (threshold_found!=0)  {
			//	refine_detection(&threshold_found, &ymin, &ymax, &xmin, &xmax, frame_BW, &ymin_final, &ymax_final, &xmin_final, &xmax_final, param);
            //}

            if (y_max_final-y_min_final>=y_min_size_text && (float)(x_max_final-x_min_final)>=ratio_width_height*(float)(y_max_final-y_min_final) && threshold_found!=0){    //if the box has the good geometry
            	y_min_final=y_min_final-2;
                x_min_final=x_min_final-2;
                y_max_final=y_max_final+2;
                x_max_final=x_max_final+2;                
                
                if (y_min_final<0)                    y_min_final=0;  
                if (x_min_final<0)                    x_min_final=0;
                if (y_max_final>=s.height)    y_max_final=s.height;
                if (x_max_final>=s.width)     x_max_final=s.width;
                
                cv::Rect roi2(x_min_final, y_min_final, x_max_final-x_min_final, y_max_final-y_min_final);
				cv::Mat im_box = frame_BW(roi2);
                cv::Size s2=im_box.size();
                cv::rectangle(im_box,cv::Point(0,0),cv::Point(s2.width-1,s2.height-1),cv::Scalar(0,0,0,0),1,4,0);
                cv::rectangle(im_box,cv::Point(1,1),cv::Point(s2.width-2,s2.height-2),cv::Scalar(0,0,0,0),1,4,0);  
                
				cv::imshow("original", im_box);
				cv::waitKey(0);
				cv::destroyWindow("original");
			     
                                                                     
                //box* pt_rect_box= create_init_box(frameNum, frameNum, (float)ymin_final, (float)xmin_final, (float)ymax_final, (float)xmax_final, threshold_found, (float)nb_cc, im_box, im_box, im_box, param);  
                //idx=0;

                //if ((seq_box->total==0) || (!cvSeqSearch(seq_box, &pt_rect_box, cmp_box, 0, &idx, frame_BW )))    cvSeqPush(seq_box, &pt_rect_box);  // if the box not in the list add the box
               // else update_box(seq_box, idx, frameNum, threshold_found, pt_rect_box, nb_cc, frame_BW, im_box, param);                               // else update the boxes position
                

            }
         
            
			 
			 
			 
		}
	}
		
    /*for(current_contour=seq_contour; current_contour!=NULL; current_contour=current_contour->h_next ) { // for each contour in contour list put it in current_contour
        cvSeqSort(current_contour, cmp_opencv_point, 0);     // compare 2 point to sort the sequence of point
         
                     
            
            if (param->text_white == 0) cvNot(frame_BW, frame_BW);
            else if (param->text_white == -1){
                double var1=0;
                double var2=0;  
                int thr_otsu_var1=0;
                int thr_otsu_var2=0; 
                var1 = noise_image(im_temp, &thr_otsu_var1, param);
                if (var1>param->max_var || var1==0.0){
                     cvNot(im_temp, im_temp); 
                     if (param->path_im_mask!=NULL) cvAnd(im_temp,im_temp_mask,im_temp, NULL);
                     var2 = noise_image(im_temp, &thr_otsu_var2, param);
                     if ((var1<var2 && var1!=0.0) || var2==0.0){
                        cvNot(im_temp, im_temp); 
                        if (param->path_im_mask!=NULL) cvAnd(im_temp,im_temp_mask,im_temp, NULL);
                     }
                     else{
                        cvNot(frame_BW, frame_BW);
                        if (param->path_im_mask!=NULL) cvAnd(frame_BW,im_mask,frame_BW, NULL);
                     }
                }
            }

           int threshold_found = -1;            
            int nb_cc=0;
            if (param->type_threshold==-1) {
                 threshold_found = cvThreshold(im_temp, im_temp, param->max_thr, param->max_thr, CV_THRESH_TOZERO+CV_THRESH_OTSU);
                 threshold_found = param->coef_increase_thr_otsu*threshold_found;
            }
            else if (param->type_threshold==-2) threshold_found = sauvola(im_temp, param->max_thr+1);
            else if (param->type_threshold==-3) threshold_found = wolf(im_temp, param->max_thr+1);
            else threshold_found=param->type_threshold;
            
            cvReleaseImage(&im_temp);
            cvReleaseImage(&im_temp_mask);
               
            if (threshold_found!=0)  refine_detection(&threshold_found, &ymin, &ymax, &xmin, &xmax, frame_BW, &ymin_final, &ymax_final, &xmin_final, &xmax_final, param);

            if (ymax_final-ymin_final>=param->y_min_size_text && (float)(xmax_final-xmin_final)>=param->ratio_width_height*(float)(ymax_final-ymin_final) && threshold_found!=0){    //if the box has the good geometry
            	ymin_final=ymin_final-2;
                xmin_final=xmin_final-2;
                ymax_final=ymax_final+2;
                xmax_final=xmax_final+2;                
                
                if (ymin_final<0)                    ymin_final=0;  
                if (xmin_final<0)                    xmin_final=0;
                if (ymax_final>=frame_BW->height)    ymax_final=frame_BW->height;
                if (xmax_final>=frame_BW->width)     xmax_final=frame_BW->width;
                cvSetImageROI(frame_BW, cvRect(xmin_final, ymin_final, xmax_final-xmin_final, ymax_final-ymin_final));
                IplImage* im_box = cvCreateImage(cvSize(xmax_final-xmin_final, ymax_final-ymin_final), frame_BW->depth, 1);
                cvCopy(frame_BW, im_box, NULL);
                cvResetImageROI(frame_BW);
                cvRectangle(im_box,cvPoint(0,0),cvPoint(im_box->width-1,im_box->height-1),cvScalar(0,0,0,0),1,4,0);
                cvRectangle(im_box,cvPoint(1,1),cvPoint(im_box->width-2,im_box->height-2),cvScalar(0,0,0,0),1,4,0);  
                                                                     
                box* pt_rect_box= create_init_box(frameNum, frameNum, (float)ymin_final, (float)xmin_final, (float)ymax_final, (float)xmax_final, threshold_found, (float)nb_cc, im_box, im_box, im_box, param);  
                idx=0;

                if ((seq_box->total==0) || (!cvSeqSearch(seq_box, &pt_rect_box, cmp_box, 0, &idx, frame_BW )))    cvSeqPush(seq_box, &pt_rect_box);  // if the box not in the list add the box
                else update_box(seq_box, idx, frameNum, threshold_found, pt_rect_box, nb_cc, frame_BW, im_box, param);                               // else update the boxes position
                

            } 

        }        
    }
*/

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

    double zoom = options.Read("--scale", 1.0); // from video.Configure()
    std::string dataPath = options.Get<std::string>("--data", "");
    std::string lang = options.Get<std::string>("--lang", "fra");
    bool upper_case = options.IsSet("--upper-case");
    bool ignore_accents = options.IsSet("--ignore-accents");
    bool sharpen = options.IsSet("--sharpen");
    bool show = options.IsSet("--show");
    std::string maskFile = options.Get<std::string>("--mask", "");
	cv::Mat mask;

	bool isMask=false;
	if (maskFile!=""){
		isMask=true;
		mask = cv::imread(maskFile, CV_LOAD_IMAGE_COLOR); 
		cv::resize(mask, mask,cv::Size(1024,576));
		}
	
	amu::OCR ocr(dataPath, lang);		    
    if(upper_case) ocr.SetUpperCase(ignore_accents);
    else ocr.SetMixedCase(ignore_accents);

    amu::VideoReader video;
    if(!video.Configure(options)) return 1;
    if(options.Size() != 0) options.Usage();
    
    
	cv::Mat image, image_BW, resized;
	image = cv::imread("/home/meriem/work/repere/data/BFMTV_BFMStory_2011-05-11_175900_13551.jpg", CV_LOAD_IMAGE_COLOR); 
	
	cv::resize(image, resized,cv::Size(1024,576));

	//while(video.HasNext()) { 
     //   if(video.ReadFrame(image)) {
	//}
	//}	
	
			// apply mask if it exists
			if (isMask) {
				cv::bitwise_and(resized, mask, resized);	
			}	
	
			cv::cvtColor(resized, image_BW, CV_RGB2GRAY);		
            cv::Mat im=image_BW;
            im=sobel_filter_H(image_BW);			
          
		    im = dilataion_erosion(im);
			im = delete_horizontal_bar(im);

			im = delete_vertical_bar(im);
	

			
			int frame = 0;
			std::vector<amu::Rect> rects = find_boxes(im, frame, im, im, im, mask,isMask);

            /*
            spatial_detection_box(im, seq_box, frameNum, frame_BW, frame, frame, im_mask, param); 	// Detect boxes spatial position
            temporal_detection_box(seq_box, seq_box_final, frameNum, frame_BW, im_mask, param);     // Temporal tracking of the boxes
 */
			
			
			
		
	
    return 0;
}
