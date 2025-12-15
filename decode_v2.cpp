#include <opencv2/opencv.hpp>
#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <algorithm>
#include <map>

// YouTube-Resistant Video Decoder
// Decodes videos created with encode_v2.cpp

namespace fs = std::filesystem;

// Configuration (must match encoder)
const int FRAME_WIDTH = 1920;
const int FRAME_HEIGHT = 1080;
const int BLOCK_SIZE = 8;
const int BLOCKS_WIDE = FRAME_WIDTH / BLOCK_SIZE;
const int BLOCKS_HIGH = FRAME_HEIGHT / BLOCK_SIZE;
const int TOTAL_BLOCKS_PER_FRAME = BLOCKS_WIDE * BLOCKS_HIGH;
const int HEADER_SIZE_BYTES = 16;

// CRC32 implementation
class CRC32 {
private:
    uint32_t table[256];

public:
    CRC32() {
        for (uint32_t i = 0; i < 256; i++) {
            uint32_t c = i;
            for (int j = 0; j < 8; j++) {
                c = (c & 1) ? (0xEDB88320 ^ (c >> 1)) : (c >> 1);
            }
            table[i] = c;
        }
    }

    uint32_t calculate(const std::vector<uint8_t>& data, size_t offset, size_t length) {
        uint32_t crc = 0xFFFFFFFF;
        for (size_t i = offset; i < offset + length && i < data.size(); i++) {
            crc = table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
        }
        return crc ^ 0xFFFFFFFF;
    }
};

// Simple Reed-Solomon decoder (matches encoder)
// Encoder format: [original_data][parity_bytes_appended_at_end]
// Parity bytes = ceil(original_size / 223) XOR parity bytes
class ReedSolomon {
public:
    static std::vector<uint8_t> decode(const std::vector<uint8_t>& data, size_t original_file_size) {
        // The encoder appends parity bytes at the END, not interleaved
        // Calculate how many parity bytes were added
        size_t chunk_size = 223;
        size_t num_parity_bytes = (original_file_size + chunk_size - 1) / chunk_size;

        // The encoded data = original_data + parity_bytes
        // So original_data_size = total_size - num_parity_bytes
        size_t data_size = data.size() - num_parity_bytes;

        if (data_size > data.size()) {
            // Fallback: no parity bytes, return as-is
            std::cerr << "Warning: Data size calculation error, returning raw data" << std::endl;
            return data;
        }

        std::cout << "  Encoded size: " << data.size() << " bytes" << std::endl;
        std::cout << "  Parity bytes: " << num_parity_bytes << std::endl;
        std::cout << "  Data portion: " << data_size << " bytes" << std::endl;

        // Extract data portion (without parity bytes at the end)
        std::vector<uint8_t> decoded(data.begin(), data.begin() + data_size);

        // Optionally verify parity (XOR parity can detect but not correct errors)
        int parity_errors = 0;
        for (size_t i = 0; i < data_size; i += chunk_size) {
            size_t len = std::min(chunk_size, data_size - i);
            uint8_t expected_parity = 0;
            for (size_t j = 0; j < len; j++) {
                expected_parity ^= data[i + j];
            }

            size_t parity_idx = data_size + (i / chunk_size);
            if (parity_idx < data.size()) {
                uint8_t actual_parity = data[parity_idx];
                if (expected_parity != actual_parity) {
                    parity_errors++;
                }
            }
        }

        if (parity_errors > 0) {
            std::cerr << "Warning: " << parity_errors << " parity check(s) failed" << std::endl;
        } else {
            std::cout << "  All parity checks passed" << std::endl;
        }

        return decoded;
    }
};

// Decode single 8x8 block to a bit using majority voting
bool decode_block_to_bit(const cv::Mat& frame, int block_idx) {
    int block_x = (block_idx % BLOCKS_WIDE) * BLOCK_SIZE;
    int block_y = (block_idx / BLOCKS_WIDE) * BLOCK_SIZE;

    if (block_y + BLOCK_SIZE > FRAME_HEIGHT || block_x + BLOCK_SIZE > FRAME_WIDTH) {
        return false;
    }

    // Count white vs black pixels (majority voting for noise resistance)
    int white_count = 0;
    int black_count = 0;

    for (int dy = 0; dy < BLOCK_SIZE; dy++) {
        for (int dx = 0; dx < BLOCK_SIZE; dx++) {
            uint8_t pixel = frame.at<uint8_t>(block_y + dy, block_x + dx);
            if (pixel > 127) {
                white_count++;
            } else {
                black_count++;
            }
        }
    }

    return white_count > black_count;
}

// Decode 8 blocks to a byte
uint8_t decode_blocks_to_byte(const cv::Mat& frame, int start_block) {
    uint8_t byte = 0;
    for (int bit_idx = 0; bit_idx < 8; bit_idx++) {
        bool bit = decode_block_to_bit(frame, start_block + bit_idx);
        if (bit) {
            byte |= (1 << (7 - bit_idx));
        }
    }
    return byte;
}

// Extract frames from video
void extract_frames_from_video(const std::string& video_file, const std::string& output_dir) {
    if (fs::exists(output_dir)) {
        fs::remove_all(output_dir);
    }
    fs::create_directories(output_dir);

    std::string ffmpeg_command = "ffmpeg -i \"" + video_file + "\" -vsync 0 \"" +
                                output_dir + "/frame_%04d.png\"";

    std::cout << "Extracting frames from video..." << std::endl;
    if (system(ffmpeg_command.c_str()) == 0) {
        std::cout << "Frames extracted to: " << output_dir << std::endl;
    } else {
        std::cerr << "Error extracting frames." << std::endl;
    }
}

// Decode all frames and reconstruct file
void decode_frames_to_file(const std::string& frames_dir,
                           const std::string& output_file,
                           int expected_file_size,
                           bool save_artifacts) {
    // Collect all frame files
    std::vector<std::string> frame_files;
    for (const auto& entry : fs::directory_iterator(frames_dir)) {
        if (entry.path().extension() == ".png") {
            frame_files.push_back(entry.path().string());
        }
    }
    std::sort(frame_files.begin(), frame_files.end());

    std::cout << "Found " << frame_files.size() << " frames to decode" << std::endl;

    CRC32 crc32;
    std::vector<uint8_t> all_data;
    int frames_with_errors = 0;

    // Decode each frame
    for (size_t frame_idx = 0; frame_idx < frame_files.size(); frame_idx++) {
        cv::Mat frame = cv::imread(frame_files[frame_idx], cv::IMREAD_GRAYSCALE);
        if (frame.empty()) {
            std::cerr << "Error reading frame: " << frame_files[frame_idx] << std::endl;
            continue;
        }

        // Decode header (16 bytes = 128 blocks)
        std::vector<uint8_t> frame_data;
        int current_block = 0;

        for (int i = 0; i < HEADER_SIZE_BYTES; i++) {
            frame_data.push_back(decode_blocks_to_byte(frame, current_block));
            current_block += 8;
        }

        // Parse header
        uint32_t stored_crc = (frame_data[0] << 24) | (frame_data[1] << 16) |
                             (frame_data[2] << 8) | frame_data[3];
        uint32_t frame_num = (frame_data[4] << 24) | (frame_data[5] << 16) |
                            (frame_data[6] << 8) | frame_data[7];
        uint32_t total_frames = (frame_data[8] << 24) | (frame_data[9] << 16) |
                               (frame_data[10] << 8) | frame_data[11];
        uint32_t data_size = (frame_data[12] << 24) | (frame_data[13] << 16) |
                            (frame_data[14] << 8) | frame_data[15];

        // Decode data portion
        std::vector<uint8_t> data_portion;
        int bytes_per_frame = (TOTAL_BLOCKS_PER_FRAME / 8) - HEADER_SIZE_BYTES;
        int bytes_to_decode = std::min((int)data_size, bytes_per_frame);

        for (int i = 0; i < bytes_to_decode; i++) {
            data_portion.push_back(decode_blocks_to_byte(frame, current_block));
            current_block += 8;
        }

        // Verify CRC
        uint32_t calculated_crc = crc32.calculate(data_portion, 0, data_portion.size());
        bool crc_match = (stored_crc == calculated_crc);

        if (!crc_match) {
            std::cerr << "Warning: CRC mismatch in frame " << frame_num
                     << " (stored: " << std::hex << stored_crc
                     << ", calculated: " << calculated_crc << std::dec << ")" << std::endl;
            frames_with_errors++;
        }

        // Add data to output
        all_data.insert(all_data.end(), data_portion.begin(), data_portion.end());

        if ((frame_idx + 1) % 100 == 0) {
            std::cout << "Decoded frame " << (frame_idx + 1) << "/" << frame_files.size() << std::endl;
        }
    }

    std::cout << "\nDecoding summary:" << std::endl;
    std::cout << "  Total frames: " << frame_files.size() << std::endl;
    std::cout << "  Frames with CRC errors: " << frames_with_errors << std::endl;
    std::cout << "  Total bytes decoded: " << all_data.size() << std::endl;

    // Apply Reed-Solomon decoding to recover from errors
    std::cout << "\nApplying error correction..." << std::endl;
    std::vector<uint8_t> corrected_data = ReedSolomon::decode(all_data, expected_file_size);

    std::cout << "After error correction: " << corrected_data.size() << " bytes" << std::endl;

    // Trim to expected file size if provided
    if (expected_file_size > 0 && corrected_data.size() > expected_file_size) {
        corrected_data.resize(expected_file_size);
        std::cout << "Trimmed to expected size: " << expected_file_size << " bytes" << std::endl;
    }

    // Write output file
    std::ofstream output(output_file, std::ios::binary);
    output.write(reinterpret_cast<const char*>(corrected_data.data()), corrected_data.size());
    std::cout << "\n✓ Recovered file saved as: " << output_file << std::endl;

    // Clean up artifacts
    if (!save_artifacts) {
        fs::remove_all(frames_dir);
    }
}

// Parse filename format: filename-YT-extension-filesize-v2.mkv
std::pair<std::string, int> parse_v2_filename(const std::string& video_file) {
    std::string base_name = fs::path(video_file).filename().string();

    // Look for -v2.mkv suffix
    if (base_name.find("-v2.mkv") == std::string::npos) {
        throw std::runtime_error("Not a v2 format video (expected: filename-YT-ext-size-v2.mkv)");
    }

    // Parse: filename-YT-extension-filesize-v2.mkv
    size_t last_dash = base_name.rfind("-v2.mkv");
    std::string without_suffix = base_name.substr(0, last_dash);

    size_t size_dash = without_suffix.rfind('-');
    size_t ext_dash = without_suffix.rfind('-', size_dash - 1);
    size_t yt_dash = without_suffix.rfind('-', ext_dash - 1);

    if (size_dash == std::string::npos || ext_dash == std::string::npos ||
        yt_dash == std::string::npos) {
        throw std::runtime_error("Invalid v2 filename format");
    }

    std::string filename = without_suffix.substr(0, yt_dash);
    std::string extension = without_suffix.substr(ext_dash + 1, size_dash - ext_dash - 1);
    std::string size_str = without_suffix.substr(size_dash + 1);

    int file_size = std::stoi(size_str);
    std::string output_filename = "recovered_" + filename + "." + extension;

    return {output_filename, file_size};
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <video_file> [--file-size <size>] [--file-format <ext>] [--save-artifacts]" << std::endl;
        std::cerr << "\nThis decoder recovers files from YouTube-resistant videos." << std::endl;
        std::cerr << "It can handle videos that have been re-encoded by YouTube." << std::endl;
        return 1;
    }

    std::string video_file = argv[1];
    int file_size = 0;
    std::string file_format;
    bool save_artifacts = false;

    // Parse arguments
    for (int i = 2; i < argc; ++i) {
        if (std::strcmp(argv[i], "--file-size") == 0 && i + 1 < argc) {
            file_size = std::stoi(argv[++i]);
        } else if (std::strcmp(argv[i], "--file-format") == 0 && i + 1 < argc) {
            file_format = argv[++i];
        } else if (std::strcmp(argv[i], "--save-artifacts") == 0) {
            save_artifacts = true;
        }
    }

    std::cout << "YouTube-Resistant Video Decoder v2" << std::endl;
    std::cout << "===================================" << std::endl;

    std::string output_filename;

    // Try to infer from filename
    if (file_size == 0 || file_format.empty()) {
        try {
            auto [inferred_name, inferred_size] = parse_v2_filename(video_file);
            output_filename = inferred_name;
            file_size = inferred_size;
            std::cout << "Inferred: " << output_filename << " (" << file_size << " bytes)" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            std::cerr << "Please provide --file-size and --file-format" << std::endl;
            return 1;
        }
    } else {
        output_filename = "recovered_file." + file_format;
    }

    // Extract frames
    std::string frames_dir = "decoded_images_v2";
    extract_frames_from_video(video_file, frames_dir);

    // Decode frames to file
    decode_frames_to_file(frames_dir, output_filename, file_size, save_artifacts);

    return 0;
}
