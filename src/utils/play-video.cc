// Play videos (repere videos too) 
// Please read README.txt for information and build instructions.
#include <opencv2/opencv.hpp>
#include "video.h"
#include "commandline.h"



// Main function: plots frames and prints its corresponding num_frame and time
int main(int argc, char** argv) {
    amu::CommandLine options(argv, "[options]\n");

    amu::VideoReader video;
    if(!video.Configure(options)) return 1;
    if(options.Size() != 0) options.Usage();

    amu::Player player(video);
    while(player.Next()) {
        if(player.Playing()) std::cout << video.GetIndex() << " " << video.GetTime() << "\n";
        player.Show();
    }

}
