#include <iostream>
#include <list>

#include "repere.h"
#include "commandline.h"
#include "face.h"

namespace amu {
    struct Detection {
        int frame;
        cv::Rect location;
        double skin;
        int model;
        // read face detections
        static std::vector<Detection> Load(const std::string& filename) {
            std::vector<Detection> output;
            std::ifstream input(filename.c_str());
            if(input) {
                std::string line;
                while(std::getline(input, line)) {
                    std::stringstream reader(line);
                    Detection detection;
                    reader >> detection.frame >> detection.location.x >> detection.location.y >> detection.location.width >> detection.location.height >> detection.skin >> detection.model;
                    if(detection.model < 2 && detection.skin > .3) 
                        output.push_back(detection);
                }
            } else {
                std::cerr << "ERROR: loading \"" << filename << "\"\n";
            }
            return output;
        }
        
         // read face detections and sort by frame 
        static std::map<int, std::vector<Detection> > LoadByFrame(const std::string& filename) {
            std::vector<Detection> detections = Load(filename);
            std::map<int, std::vector<Detection> > output;
            for(std::vector<Detection>::const_iterator i = detections.begin(); i != detections.end(); i++) {
                output[i->frame].push_back(*i);
            }
            return output;
        }
    };

}

int main(int argc, char** argv) {

    amu::CommandLine options(argv, "[options]\n");
    options.AddUsage("  --shots <shot-file>               shot segmentation output\n");
    options.AddUsage("  --detections <detection-file>     haar detector ourput for each frame\n");
    options.AddUsage("  --idx <idx-file>                  idx for converting frames to times\n");
    options.AddUsage("  --min-detect <int>                minimum number of detections in track (default=5)\n");
    options.AddUsage("  --min-density <float>             minimum density of detections in track (default=0.5)\n");

    std::string shotFilename = options.Get<std::string>("--shots", "");
    std::string detectionFilename = options.Get<std::string>("--detections", "");
    std::string idxFilename = options.Get<std::string>("--idx", "");
    int minDetections = options.Get("--min-detect", 5);
    double minDensity = options.Get("--min-density", 0.5);

    if(options.Size() != 0 || shotFilename == "" || detectionFilename == "" )  options.Usage();
    std::vector<amu::ShotSegment> shots;
    shots = amu::ShotSegment::Read(shotFilename);
    std::string showname = amu::ShowName(detectionFilename);
    std::map<int, std::vector<amu::Detection> > detections = amu::Detection::LoadByFrame(detectionFilename);
    typedef std::map<int, amu::Detection> Track;
	std::cout <<shots.size() <<std::endl;
	
	
	std::vector<amu::Detection> detections2 = amu::Detection::Load(detectionFilename);
    int num = 0;
    
    
    
    for(std::vector<amu::ShotSegment>::iterator shot = shots.begin(); shot != shots.end(); shot++) {
        std::map<int, std::vector<amu::Detection> >::iterator frame = detections.lower_bound(shot->startFrame);
        
        if(frame == detections.end() || frame->first < shot->startFrame || frame->first > shot->endFrame) continue;
        std::list<Track> tracks;
        while(frame != detections.end() && frame->first < shot->endFrame) {
            for(std::vector<amu::Detection>::iterator detection = frame->second.begin(); detection != frame->second.end(); detection++) {
                bool found = false;
                for(std::list<Track>::iterator track = tracks.begin(); track != tracks.end(); track++) {
                    const amu::Detection &other = track->rbegin()->second;
                    cv::Rect intersection = detection->location & other.location;
                    if(intersection == detection->location || intersection == other.location 
                            || intersection.area() > .8 * detection->location.area() || intersection.area() > .8 * other.location.area()) {
                        found = true;
                        if(track->find(detection->frame) == track->end() || (*track)[detection->frame].model > detection->model) {
                            (*track)[detection->frame] = *detection;
                        }
                        break;
                    }
                }
                if(!found) {
                    Track track;
                    track[detection->frame] = *detection;
                    tracks.push_back(track);
                }
            }
            frame++;
        }
        for(std::list<Track>::iterator track = tracks.begin(); track != tracks.end(); track++) {
            double density = (double)track->size() / (shot->endFrame - shot->startFrame);
            if(track->size() < minDetections || density < minDensity) continue;
            std::cout << showname << " " << shot->startTime << " " << shot->endTime << " head " << shot->id << ":Inconnu_" << num;
            for(Track::iterator iterator = track->begin(); iterator != track->end(); iterator++) {
                const amu::Detection& detection = iterator->second;
                std::cout << " || frame=" << detection.frame << " x=" << detection.location.x << " y=" << detection.location.y 
                    << " w=" << detection.location.width << " h=" << detection.location.height 
                    << " model=" << detection.model;
            }
            std::cout << "\n";
            num += 1;
        }
    }
    return 0;
}
