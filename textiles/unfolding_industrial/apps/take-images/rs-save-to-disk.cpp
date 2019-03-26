// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015-2017 Intel Corporation. All Rights Reserved.

#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API

#include <fstream>              // File IO
#include <iostream>             // Terminal IO
#include <sstream>              // Stringstreams

// 3rd party header for writing png files
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <wiringPi.h>
#include <errno.h>
#include <unistd.h>

// Interrupt pins
static const int INTERRUPT_PIN_0 = 0;
static const int INTERRUPT_PIN_1 = 2;

// Global variables (use with caution)
volatile int captureFlag = false;
volatile int nextObjectFlag = false;

void captureInterrupt(void)
{
    captureFlag = true;
}

void nextObjectInterrupt(void)
{
    nextObjectFlag = true;
}

// Helper function for writing metadata to disk as a csv file
void metadata_to_csv(const rs2::frame& frm, const std::string& filename);

// This sample captures 30 frames and writes the last frame to disk.
// It can be useful for debugging an embedded system with no display.
int main(int argc, char * argv[]) try
{
    //----- WiringPi Configuration -----------------------------------------------
    if (wiringPiSetup() < 0)
    {
        std::cerr << "Unable to setup WiringPi: " << strerror(errno) << std::endl;
        return 1;
    }

    if (wiringPiISR(INTERRUPT_PIN_0, INT_EDGE_RISING, &captureInterrupt) < 0)
    {
        std::cerr << "Unable to setup ISR: " << strerror(errno) << std::endl;
        return 1;
    }

    if (wiringPiISR(INTERRUPT_PIN_1, INT_EDGE_RISING, &nextObjectInterrupt) < 0)
    {
        std::cerr << "Unable to setup ISR: " << strerror(errno) << std::endl;
        return 1;
    }

    //----- RealSense Configuration -----------------------------------------------
    // Declare depth colorizer for pretty visualization of depth data
    rs2::colorizer color_map;

    // Declare RealSense pipeline, encapsulating the actual device and sensors
    rs2::pipeline pipe;
    // Start streaming with default recommended configuration
    pipe.start();

    // Capture 30 frames to give autoexposure, etc. a chance to settle
    for (auto i = 0; i < 30; ++i) pipe.wait_for_frames();



    int counter = 0;

    while(!nextObjectFlag)
    {
        for (auto&& frame : pipe.wait_for_frames())
        {
            std::cout << "A new frame has arrived!" << std::endl;

            // Wait for the next set of frames from the camera. Now that autoexposure, etc.
            // has settled, we will write these to disk
            if (captureFlag)
            {
                std::cout << "Capture requested!" << std::endl;

                // We can only save video frames as pngs, so we skip the rest
                if (auto vf = frame.as<rs2::video_frame>())
                {
                    std::cout << "Correct frame, saving it..." << std::endl;

                    auto stream = frame.get_profile().stream_type();
                    // Use the colorizer to get an rgb image for the depth stream
                    if (vf.is<rs2::depth_frame>()) vf = color_map.process(frame);

                    // Write images to disk
                    std::stringstream png_file;
                    png_file << "rs-save-to-disk-output-" << vf.get_profile().stream_name() << "-"<< counter << ".png";
                    stbi_write_png(png_file.str().c_str(), vf.get_width(), vf.get_height(),
                                   vf.get_bytes_per_pixel(), vf.get_data(), vf.get_stride_in_bytes());
                    std::cout << "Saved " << png_file.str() << std::endl;

                    // Record per-frame metadata for UVC streams
                    std::stringstream csv_file;
                    csv_file << "rs-save-to-disk-output-" << vf.get_profile().stream_name()
                             << "-metadata" << "-"<< counter << ".csv";
                    metadata_to_csv(vf, csv_file.str());
                }

                captureFlag = false;
                counter++;
            }
        }
        usleep(100);
    }

    return EXIT_SUCCESS;
}
catch(const rs2::error & e)
{
    std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
    return EXIT_FAILURE;
}
catch(const std::exception & e)
{
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}

void metadata_to_csv(const rs2::frame& frm, const std::string& filename)
{
    std::ofstream csv;

    csv.open(filename);

    //    std::cout << "Writing metadata to " << filename << endl;
    csv << "Stream," << rs2_stream_to_string(frm.get_profile().stream_type()) << "\nMetadata Attribute,Value\n";

    // Record all the available metadata attributes
    for (size_t i = 0; i < RS2_FRAME_METADATA_COUNT; i++)
    {
        if (frm.supports_frame_metadata((rs2_frame_metadata_value)i))
        {
            csv << rs2_frame_metadata_to_string((rs2_frame_metadata_value)i) << ","
                << frm.get_frame_metadata((rs2_frame_metadata_value)i) << "\n";
        }
    }

    csv.close();
}