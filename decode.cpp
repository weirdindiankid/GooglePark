#include <opencv2/opencv.hpp>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <filesystem>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <sys/stat.h>
#include <unistd.h>

// Alias filesystem namespace for portability
namespace fs = std::__fs::filesystem; // or use std::filesystem if available

// Clear or create a directory
void clear_or_create_dir(const std::string& directory) {
    if (fs::exists(directory)) {
        fs::remove_all(directory);
    }
    fs::create_directories(directory);
}

// Extract frames from video using FFmpeg
void extract_frames_from_video(const std::string& video_file, const std::string& output_dir) {
    clear_or_create_dir(output_dir);
    
    // FFmpeg command to extract frames
    std::string ffmpeg_command = "ffmpeg -i " + video_file + " -vsync 0 " + output_dir + "/frame_%04d.png";
    
    if (system(ffmpeg_command.c_str()) == 0) {
        std::cout << "Frames extracted to: " << output_dir << std::endl;
    } else {
        std::cerr << "Error extracting frames." << std::endl;
    }
}

// Infer file details from the video filename (format: filename-fileextension-filesize.mkv)
std::pair<std::string, int> infer_file_details_from_filename(const std::string& video_filename) {
    std::string base_name = fs::path(video_filename).filename().string();
    size_t last_dash_pos = base_name.rfind('-');
    size_t second_last_dash_pos = base_name.rfind('-', last_dash_pos - 1);
    size_t dot_pos = base_name.rfind('.');

    // Ensure the file ends with ".mkv", has two dashes and the last part is numeric
    if (dot_pos == std::string::npos || base_name.substr(dot_pos + 1) != "mkv" ||
        last_dash_pos == std::string::npos || second_last_dash_pos == std::string::npos) {
        throw std::runtime_error("Filename " + base_name + " is not in the expected format 'filename-fileextension-filesize.mkv'");
    }

    std::string filename = base_name.substr(0, second_last_dash_pos);
    std::string file_extension = base_name.substr(second_last_dash_pos + 1, last_dash_pos - second_last_dash_pos - 1);

    // Attempt to parse the size (last part before ".mkv") as an integer
    int file_size;
    try {
        file_size = std::stoi(base_name.substr(last_dash_pos + 1, dot_pos - last_dash_pos - 1));
    } catch (const std::invalid_argument&) {
        throw std::runtime_error("Unable to parse file size from the filename: " + base_name);
    }

    std::string original_filename = "recovered_" + filename + "." + file_extension;
    return {original_filename, file_size};
}

// Convert extracted frames back into the original file
void images_to_file(const std::string& input_dir, const std::string& output_file, cv::Size frame_size, int original_length, bool save_artifacts) {
    std::vector<unsigned char> decoded_data;

    // Collect all image files
    std::vector<std::string> image_files;
    for (const auto& entry : fs::directory_iterator(input_dir)) {
        if (entry.path().extension() == ".png") {
            image_files.push_back(entry.path().string());
        }
    }
    std::sort(image_files.begin(), image_files.end());

    for (const std::string& image_file : image_files) {
        cv::Mat frame = cv::imread(image_file, cv::IMREAD_GRAYSCALE);
        if (frame.empty()) {
            std::cerr << "Error reading image: " << image_file << std::endl;
            continue;
        }
        decoded_data.insert(decoded_data.end(), frame.begin<unsigned char>(), frame.end<unsigned char>());
    }

    // Trim to original length
    decoded_data.resize(original_length);

    // Write the data back to a file
    std::ofstream output(output_file, std::ios::binary);
    output.write(reinterpret_cast<const char*>(decoded_data.data()), original_length);
    std::cout << "Recovered file saved as " << output_file << std::endl;

    if (!save_artifacts) {
        fs::remove_all(input_dir);
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <video_file> [--file-size <size>] [--file-format <format>] [--save-artifacts]" << std::endl;
        return 1;
    }

    std::string video_file = argv[1];
    int file_size = 0;
    std::string file_format;
    bool save_artifacts = false;

    // Parse optional arguments
    for (int i = 2; i < argc; ++i) {
        if (std::strcmp(argv[i], "--file-size") == 0 && i + 1 < argc) {
            file_size = std::stoi(argv[++i]);
        } else if (std::strcmp(argv[i], "--file-format") == 0 && i + 1 < argc) {
            file_format = argv[++i];
        } else if (std::strcmp(argv[i], "--save-artifacts") == 0) {
            save_artifacts = true;
        }
    }

    std::string original_filename;
    if (file_size > 0 && !file_format.empty()) {
        original_filename = "recovered_file." + file_format;
        std::cout << "Using provided size and format: " << original_filename << ", Size: " << file_size << " bytes" << std::endl;
    } else {
        try {
            auto [inferred_filename, inferred_size] = infer_file_details_from_filename(video_file);
            original_filename = inferred_filename;
            file_size = inferred_size;
            std::cout << "Inferred original file: " << original_filename << ", Size: " << file_size << " bytes" << std::endl;
        } catch (const std::runtime_error& e) {
            std::cerr << e.what() << std::endl;
            std::cerr << "Error: Please provide --file-size and --file-format or rename the input file to follow the format 'filename-fileextension-filesize.mkv'." << std::endl;
            return 1;
        }
    }

    // Step 2: Extract frames from video
    std::string output_images_dir = "decoded_images";
    extract_frames_from_video(video_file, output_images_dir);

    // Step 3: Reconstruct the original file from the frames
    images_to_file(output_images_dir, original_filename, cv::Size(640, 480), file_size, save_artifacts);

    return 0;
}
