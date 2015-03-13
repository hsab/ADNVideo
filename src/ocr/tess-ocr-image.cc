
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>

// opencv
#include <cv.h>
#include <highgui.h>

// tesseract
#include <tesseract/baseapi.h>
#include <tesseract/resultiterator.h>

int main(int argc, char** argv) {
    tesseract::TessBaseAPI tess; 
    setenv("TESSDATA_PREFIX", std::string("/home/meriem/work/repere/OCR/tessdata").c_str(), 1);
    //tess.Init("OCR", "fra", tesseract::OEM_DEFAULT); 
    tess.Init(NULL,"fra", tesseract::OEM_DEFAULT);

    tess.SetVariable("tessedit_char_whitelist", "!\"$%&'()+,-./0123456789:;?@ABCDEFGHIJKLMNOPQRSTUVWXYZ\\_abcdefghijklmnopqrstuvwxyz «°»");

    cv::Mat image = cv::imread(argv[1]);
    cv::resize(image, image, cv::Size(1024 * 4, 576 * 4));
    cv::cvtColor(image, image, CV_BGR2GRAY);
    cv::threshold(image, image, 128, 255, cv::THRESH_BINARY|cv::THRESH_OTSU);

    tess.SetImage((uchar*) image.data, image.size().width, image.size().height, image.channels(), image.step1());
    //tess.SetRectangle(rect.x, rect.y, rect.width, rect.height);
    tess.Recognize(NULL);
    const char* text = tess.GetUTF8Text();
    
    std::cout << "hola" << "\n";

    std::cout << tess.MeanTextConf() / 100.0 << " " << text << "\n";
    cv::Mat resized;
    cv::resize(image, resized, cv::Size(1024, 576 ));
    cv::imshow("image", resized);
    cv::waitKey(0);
}
