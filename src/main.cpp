#include <iostream>
#include <stdlib.h>
#include <map>

#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include "json.hpp"

using namespace cv;

#define MAIN_WINDOW_NAME "Main"
#define PBSTR "||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||"
#define PBWIDTH 60
#define MIN_TEMPLATE_DETECTED_DURATION .5 // Seconds

int CHECK_EVERYTH_FRAME = 1;
int match_method = TM_CCOEFF_NORMED;
double thresh = 0.8;

void printProgress(double);
bool cvMatEQ(Mat, Mat);

int main(int argc, char** argv){
    if(argc < 6){
        std::cerr << "Usage: extractor video.mp4 template.png 3 3 1 1" << std::endl;
        return 1;
    }

    // Videofile
    std::string video_filename = argv[1];
    // Template image
    std::string template_filename = argv[2];

    // Additional info
    // - Maximum amount of templates allowed
    unsigned int max_templates_allowed = 1;
    if(argc == 6){
        max_templates_allowed = atoi(argv[5]);
    }
    // - Debug
    unsigned int debug = 0;
    if(argc == 7){
        debug = atoi(argv[6]);
    }
    // - Time bounds shift
    int seconds_before_appear = atoi(argv[3]);
    int seconds_after_appear = atoi(argv[4]);

    // Read template
    Mat template_image = imread(template_filename, IMREAD_GRAYSCALE);
    if( !template_image.data ){
        std::cerr << "Template image cannot be opened" << '\n';
        return 1;
    }

    VideoCapture capture(video_filename);
    Mat frame, prev_frame;

    if( !capture.isOpened() ){
        std::cerr << "Video cannot be opened" << '\n';
        return 1;
    }

    cvNamedWindow(MAIN_WINDOW_NAME, 1);
    cvMoveWindow(MAIN_WINDOW_NAME, 0, 0);

    // Meta info
    int fps = (int)capture.get(CV_CAP_PROP_FPS);
    int frames_counter = 0;
    int total_frames = (int)capture.get(CV_CAP_PROP_FRAME_COUNT);
    CHECK_EVERYTH_FRAME = fps/10;

    // Vector of template
    std::vector<int> template_appear_frames;

    while(true){
        capture >> frame;
        if(frame.empty()){
            break;
        }
        frames_counter++;

        Mat frame_copy;
        cvtColor(frame, frame_copy, CV_BGR2GRAY);

        printProgress(frames_counter/double(total_frames));

        if(frames_counter%CHECK_EVERYTH_FRAME != 0 ){
            continue;
        }

        // If this is the frame as previous - skip it
        if(cvMatEQ(prev_frame, frame) == true){
            continue;
        }

        // Work
        Mat img_display; Mat resb; Mat result;

        frame_copy.copyTo( img_display );

        matchTemplate( frame_copy, template_image, result, match_method );
        threshold(result, result, thresh, 1., THRESH_BINARY);
        result.convertTo(resb, CV_8U, 255);

        std::vector<std::vector<Point>> contours;
        findContours(resb, contours, RETR_LIST, CHAIN_APPROX_SIMPLE);

        // Only one concede button allowed :D
        if(contours.size() > 0 && contours.size() <= max_templates_allowed ){
            template_appear_frames.push_back(
                frames_counter
            );
        }

        if(debug == 1){
            for (size_t i=0; i<contours.size(); i++){
              Mat1b mask(result.rows, result.cols, uchar(0));
              drawContours(mask, contours, i, Scalar(255), CV_FILLED);

              Point max_point;
              double max_val;
              minMaxLoc(result, NULL, &max_val, NULL, &max_point, mask);

              rectangle(img_display, Rect(max_point.x, max_point.y, template_image.cols, template_image.rows), Scalar(255,0,255), 2);
            }

            resize(img_display, img_display, Size(1000, 600));
            imshow( MAIN_WINDOW_NAME, img_display );
            waitKey(1);
        }

        prev_frame = frame.clone();
    }

    // Remove small frame intervals
    if(template_appear_frames.size() > 1){
        int border_frame_position = 0;
        std::vector<int> frames_to_remove;
        for(size_t i = 1; i < template_appear_frames.size(); i++){
            double border_second = template_appear_frames[border_frame_position] / double(fps);
            double appear_second = template_appear_frames[i] / double(fps);

            double seconds_diff = abs(appear_second - border_second);
            if( seconds_diff <= MIN_TEMPLATE_DETECTED_DURATION ){
                frames_to_remove.push_back(i);
            }
            border_frame_position = i;
        }
        for(size_t i = 0; i < frames_to_remove.size(); i++){
            int frame_number = frames_to_remove[i];
            template_appear_frames[frame_number] = -1;
        }
        template_appear_frames.erase(
            std::remove(template_appear_frames.begin(), template_appear_frames.end(), -1),
            template_appear_frames.end()
        );
    };


    // Convert frame numbers to timestamps
    std::vector<std::vector<std::string>> template_appear_timestamps;

    if(template_appear_frames.size() > 0){
        for(size_t i=0; i<template_appear_frames.size(); i++){
            std::vector<std::string> timestamps;

            char appear_timestamp[10], disappear_timestamp[10];
            int total_seconds = template_appear_frames[i] / fps;

            int total_seconds_before = total_seconds - seconds_before_appear;
            int total_seconds_after = total_seconds + seconds_after_appear;

            int hours = (total_seconds_before / 60 / 60) % 24;
            int minutes = (total_seconds_before / 60) % 60;
            int seconds = total_seconds_before % 60;
            sprintf(
                appear_timestamp,
                "%02d:%02d:%02d",
                hours,
                minutes,
                seconds
            );

            hours = (total_seconds_after / 60 / 60) % 24;
            minutes = (total_seconds_after / 60) % 60;
            seconds = total_seconds_after % 60;
            sprintf(
                disappear_timestamp,
                "%02d:%02d:%02d",
                hours,
                minutes,
                seconds
            );

            timestamps.push_back(appear_timestamp);
            timestamps.push_back(disappear_timestamp);
            template_appear_timestamps.push_back(timestamps);
        }
    }

    std::cout << std::endl << "Results: " << std::endl;
    nlohmann::json j_vector(template_appear_timestamps);
    std::cout << j_vector.dump(4) << std::endl;
}

void printProgress (double percentage){
    int val = (int) (percentage * 100);
    int lpad = (int) (percentage * PBWIDTH);
    int rpad = PBWIDTH - lpad;
    printf ("\r%3d%% [%.*s%*s]", val, lpad, PBSTR, rpad, "");
    fflush (stdout);
}

bool cvMatEQ(Mat data1, Mat data2){
    bool success = true;
    // check if is multi dimensional
    if(data1.dims > 2 || data2.dims > 2){
        if( data1.dims != data2.dims || data1.type() != data2.type() ){
          return false;
        }
        for(int32_t dim = 0; dim < data1.dims; dim++){
          if(data1.size[dim] != data2.size[dim]){
            return false;
          }
        }
    }else{
        if(data1.size() != data2.size() || data1.channels() != data2.channels() || data1.type() != data2.type()){
          return false;
        }
    }
    int nrOfElements = data1.total()*data1.elemSize1();
    //bytewise comparison of data
    int cnt = 0;
    for(cnt = 0; cnt < nrOfElements && success; cnt++){
        if(data1.data[cnt] != data2.data[cnt]){
          success = false;
        }
    }
    return success;
}
