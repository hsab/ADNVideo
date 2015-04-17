#include <map>

#include "repere.h"
#include "xml.h"
#include "face.h"
#include <floatfann.h>

namespace amu {
    class FannClassifier {
        private:
            struct fann* ann;
        public:
            FannClassifier(const std::string& filename = "") : ann(NULL) {
                if(filename != "") Load(filename);
            }
            ~FannClassifier() {
                if(ann != NULL) fann_destroy(ann);
            }

            bool Load(const std::string& filename) {
                ann = fann_create_from_file(filename.c_str());
                return ann != NULL;
            }

            double Classify(const std::vector<float>& descriptors) const {
                fann_type input[descriptors.size()];
                for(size_t i = 0; i < descriptors.size(); i++) {
                    input[i] = descriptors[i];
                }
                fann_type* output = fann_run(ann, input);
                return output[0];
            }
    };
}

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

bool ReadLibLinearModel(const std::string& filename, std::vector<float>& model) {
    std::fstream modelStream(filename.c_str());
    if(!modelStream) {
        std::cerr << "ERROR: could not load hog model \"" << filename << "\"\n";
        return false;
    }
    model.clear();
    std::string line;
    bool keepWeights = false;
    while(std::getline(modelStream, line)) {
        if(line == "w") keepWeights = true;
        else if(keepWeights) {
            std::stringstream reader(line);
            float value;
            reader >> value;
            model.push_back(value);
        }
    }
    return true;
}

struct Person {
    std::string name;
    int keyFrame;
    int startFrame;
    int endFrame;
    cv::Rect rect;
    std::vector<cv::Point> polygon;
};

// warning: assumes valid xgtf!!!
void ReadXgtf(const char* filename, std::vector<Person>& output) {
    amu::Node* root = amu::ParseXML(filename);
    if(root == NULL) return;
    amu::Node* sourceFile = root->Find("sourcefile");
    int hSize = amu::ParseInt(sourceFile->Find("attribute", "name", "H-FRAME-SIZE")->children[0]->attributes["value"]);
    int vSize = amu::ParseInt(sourceFile->Find("attribute", "name", "V-FRAME-SIZE")->children[0]->attributes["value"]);
    std::vector<amu::Node*> found;
    root->Find("object", found, "name", "PERSONNE");
    for(size_t i = 0; i < found.size(); i++) {
        Person person;
        amu::Node* object = found[i];
        person.name = object->Find("attribute", "name", "NOM")->children[0]->attributes["value"];
        person.startFrame = amu::ParseInt(object->Find("attribute", "name", "STARTFRAME")->children[0]->attributes["value"]);
        person.endFrame = amu::ParseInt(object->Find("attribute", "name", "ENDFRAME")->children[0]->attributes["value"]);
        person.keyFrame = amu::ParseInt(object->attributes["framespan"]);
        std::vector<amu::Node*> points;
        object->Find("data:point", points);
        for(size_t j = 0; j < points.size(); j++) {
            int x = amu::ParseInt(points[j]->attributes["x"]) * 1024 / hSize;
            int y = amu::ParseInt(points[j]->attributes["y"]) * 576 / vSize;
            if(person.polygon.size() == 0 || person.rect.x > x) person.rect.x = x; // compute bounding box
            if(person.polygon.size() == 0 || person.rect.y > y) person.rect.y = y;
            if(person.polygon.size() == 0 || person.rect.width < x) person.rect.width = x;
            if(person.polygon.size() == 0 || person.rect.height < y) person.rect.height = y;
            person.polygon.push_back(cv::Point(x, y));
        }
        person.rect.width -= person.rect.x;
        person.rect.height -= person.rect.y;
        output.push_back(person);
    }
    delete root;
}

int main(int argc, char** argv) {
    amu::CommandLine options(argv, "[options]\n");
    options.AddUsage("  --xgtf <reference-xgtf>           xgtf file containing face polygons\n");
    options.AddUsage("  --window <size>                   size of hog classifier (default=32)\n");
    options.AddUsage("  --grow <ratio>                    grow reference by ratio (default=1.5)\n");
    options.AddUsage("  --model <model-file>              add negatives from wrong detections by previous model \n");
    options.AddUsage("  --groups <num>                    minimum number of detections that need to be clustered together to fire (default=2)\n");
    options.AddUsage("  --threshold <threshold>           score decision threshold (default=0)\n");
    options.AddUsage("  --show                            show detections\n");
    std::string xgtfFilename = options.Get<std::string>("--xgtf", "");
    int window = options.Get("--window", 32);
    double grow = options.Get("--grow", 1.5);
    std::string hogModelFile = options.Get<std::string>("--model", "");
    double hitThreshold = options.Get("--threshold", 0.0);
    int groupThreshold = options.Get("--groups", 2);
    double scale = options.Read("--scale", 1.0);
    bool show = options.IsSet("--show");

    amu::VideoReader video;
    if(!video.Configure(options)) return 1;
    if(options.Size() != 0 || xgtfFilename == "" || hogModelFile == "" ) options.Usage();

    std::vector<Person> persons;
    ReadXgtf(xgtfFilename.c_str(), persons);

    cv::HOGDescriptor hog(cv::Size(window, window), cv::Size(16,16), cv::Size(8,8), cv::Size(8,8), 9);

    if(hogModelFile != "") {
        std::vector<float> model;
        ReadLibLinearModel(hogModelFile, model);
        hog.setSVMDetector(model);
    }

    std::map<int, std::vector<int> > frames;
    for(size_t p = 0; p < persons.size(); p++) {
        frames[persons[p].keyFrame].push_back(p);
    }

    cv::Mat image;
    cv::Size size = video.GetSize();
    for(std::map<int, std::vector<int> >::iterator frame = frames.begin(); frame != frames.end(); frame++) {
        video.Seek(frame->first);
        video.ReadFrame(image);
        std::vector<cv::Rect> found;
        cv::Size padding(cv::Size(0, 0));
        cv::Size winStride(cv::Size(4, 4));
        hog.detectMultiScale(image, found, hitThreshold, winStride, padding, 1.1, groupThreshold);
        for(size_t i = 0; i < found.size(); i++) {    
            //if(show) cv::rectangle(image, found[i], cv::Scalar(0, 0, 255), 1);       
            std::cout <<found[i].x << " " <<found[i].y  << " " <<found[i].width  << " " <<found[i].height <<std::endl;

        }      
        if(show) {
            cv::imshow("image", image);
            cv::waitKey(0);
        }

    }
}
