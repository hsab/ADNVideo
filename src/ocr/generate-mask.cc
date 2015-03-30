#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <iostream>



using namespace cv;
using namespace std;
bool ok=true;
bool destroy=false;
Rect box;
bool drawing_box = false;



void my_mouse_callback( int event, int x, int y, int flags, void* param );


void draw_box(Mat * img, Rect rect){
  rectangle(*img, Point(box.x, box.y), Point(box.x+box.width,box.y+box.height),Scalar(0,0,255) ,2);
  Rect rect2=Rect(box.x,box.y,box.width,box.height);
}

// Implement mouse callback
void my_mouse_callback( int event, int x, int y, int flags, void* param){
  Mat* frame = (Mat*) param;
  switch( event )  {
      case CV_EVENT_MOUSEMOVE: {
          if( drawing_box ) {
              box.width = x-box.x;
              box.height = y-box.y;
          }
		}
		
      break;
      case CV_EVENT_LBUTTONDOWN: {   
		  drawing_box = true;
          box = Rect( x, y, 0, 0 );
		}
      break;

      case CV_EVENT_LBUTTONUP: {   
		  drawing_box = false;
          if( box.width < 0 )  {   
			  box.x += box.width;
              box.width *= -1;      
			}

          if( box.height < 0 ){   
			  box.y += box.height;
              box.height *= -1; 
			}
              draw_box(frame, box); 
          }
          
          break;
		case CV_EVENT_RBUTTONDOWN: {
			Size s = frame->size();
			Mat atom_image = Mat::zeros(s.height,s.width, CV_8UC3 );
			rectangle(atom_image,  box, Scalar(255,255,255), -1, 8, 0);
			imwrite("images/mask.png", atom_image);
			ok=false;
			cv::destroyWindow("Box Example");
			break; 
			}
		   break; 	
          
          
      break;

      default:
      break;
   }
}

   
    
    
    
int main() {
	  String name = "Box Example";
	  namedWindow( name );
	  box = Rect(0,0,1,1);
	  Mat image = imread("../repere/data/vlcsnap-2015-03-30-12h17m52s24.png");
	  Mat temp = image.clone();
	  
	  setMouseCallback(name, my_mouse_callback, &image);
	  while(ok){
		temp = image.clone();
		if (drawing_box) draw_box(&temp, box);
		imshow(name, temp);
		if (waitKey(15) ==0)
		break;
	  }
	  
	  return 0;
}
