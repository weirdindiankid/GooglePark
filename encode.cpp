#include <opencv2/opencv.hpp>
#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>
#include <unistd.h>

// Function to convert a file to images
void file_to_images(const std::string& input_file, const std::string& output_dir, cv::Size frame_size = cv::Size(640, 480)) {
    // Extract filename and extension
    std::string filename = input_file.substr(input_file.find_last_of("/\\") + 1);
    std::string file_extension = filename.substr(filename.find_last_of(".") + 1);
    filename = filename.substr(0, filename.find_last_of("."));

    // Read the binary file
    std::ifstream file(input_file, std::ios::binary | std::ios::ate);
    std::streamsize file_size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<unsigned char> file_data(file_size);
    if (!file.read((char*)file_data.data(), file_size)) {
        std::cerr << "Failed to read file." << std::endl;
        return;
    }

    int total_pixels = frame_size.width * frame_size.height;
    int padding_length = total_pixels - (file_size % total_pixels);
    file_data.resize(file_data.size() + padding_length, 0); // Padding the file data

    std::filesystem::remove_all(output_dir);
    std::filesystem::create_directories(output_dir);

    // Reshape data into frames
    int total_frames = file_data.size() / total_pixels;
    for (int i = 0; i < total_frames; i++) {
        cv::Mat frame(frame_size.height, frame_size.width, CV_8U, &file_data[i * total_pixels]);
        std::string image_path = output_dir + "/frame_" + std::to_string(i) + ".png";
        cv::imwrite(image_path, frame);
    }

    // Save metadata
    std::ofstream metadata_file("meta.json");
    metadata_file << "{\"" << filename << "." << file_extension << "\": " << file_size << "}" << std::endl;
    std::cout << "Frames saved to " << output_dir << " and metadata saved to meta.json in the current directory." << std::endl;
}

// Function to create a lossless video using FFmpeg
void create_lossless_video(const std::string& input_images_dir, const std::string& output_video, bool save_output_images) {
    std::string ffmpeg_command = "ffmpeg -framerate 24 -i " + input_images_dir + "/frame_%d.png -c:v ffv1 -q:v 0 " + output_video;
    if (system(ffmpeg_command.c_str()) == 0) {
        std::cout << "Lossless video created: " << output_video << std::endl;
        if (!save_output_images) {
            std::filesystem::remove_all(input_images_dir);
        }
    } else {
        std::cerr << "Error running FFmpeg command." << std::endl;
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <input_file> [--output-dir <output_dir>] [--save-artifacts]" << std::endl;
        return 1;
    }

    std::string input_file = argv[1];
    std::string output_images_dir = "output_images";
    std::string output_path_dir = ".";
    bool save_artifacts = false;

    // Get file size and format (extension)
    std::ifstream file(input_file, std::ios::binary | std::ios::ate);
    std::streamsize file_size = file.tellg();
    std::string file_extension = input_file.substr(input_file.find_last_of(".") + 1);

    for (int i = 2; i < argc; ++i) {
        if (std::strcmp(argv[i], "--output-dir") == 0 && i + 1 < argc) {
            output_path_dir = argv[++i];
        } else if (std::strcmp(argv[i], "--save-artifacts") == 0) {
            save_artifacts = true;
        }
    }

    std::filesystem::create_directories(output_images_dir);
    file_to_images(input_file, output_images_dir);

    // Extract filename without extension
    std::string filename = input_file.substr(input_file.find_last_of("/\\") + 1);
    filename = filename.substr(0, filename.find_last_of("."));

    // Generate video filename with format and file size included
    std::string video_filename = filename + "-" + file_extension + "-" + std::to_string(file_size) + ".mkv";
    std::string output_video_path = output_path_dir + "/" + video_filename;

    // Create lossless video
    create_lossless_video(output_images_dir, output_video_path, save_artifacts);

    return 0;
}
