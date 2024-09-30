import numpy as np
import cv2
import os
import subprocess
import shutil
import json
import sys
import argparse

def file_to_images(input_file, output_dir, frame_size=(640, 480)):
    # Extract the filename and extension from the input path
    filename, file_extension = os.path.splitext(os.path.basename(input_file))
    file_extension = file_extension.lstrip(".")  # Remove the leading dot from the extension

    # Read the binary data from the file
    with open(input_file, 'rb') as f:
        file_data = f.read()

    data_length = len(file_data)
    total_pixels = frame_size[0] * frame_size[1]  # Total number of pixels per image

    # Calculate padding
    padding_length = total_pixels - (data_length % total_pixels) if data_length % total_pixels != 0 else 0
    padded_data = np.frombuffer(file_data + b'\x00' * padding_length, dtype=np.uint8)

    # Reshape the data into frames (1 byte = 1 pixel, grayscale)
    frames = padded_data.reshape((-1, frame_size[1], frame_size[0]))

    # Create output directory if it doesn't exist
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)
    else:
        # Empty the directory before writing anything new to it
        shutil.rmtree(output_dir)
        os.makedirs(output_dir)

    # Save each frame as a lossless PNG image
    for idx, frame in enumerate(frames):
        image_path = os.path.join(output_dir, f'frame_{idx:04d}.png')
        cv2.imwrite(image_path, frame)

    # Save the metadata as a JSON file in the current working directory
    metadata = {filename + file_extension: data_length}
    with open("meta.json", 'w') as meta_file:
        json.dump(metadata, meta_file)

    print(f"Frames saved to {output_dir} and metadata saved to meta.json in the current working directory")

    return filename, file_extension, data_length

def create_lossless_video(input_images_dir, output_video):
    # FFmpeg command to convert image sequence to lossless video using ffv1 codec
    ffmpeg_command = [
        'ffmpeg', 
        '-framerate', '24',                      # Frame rate
        '-i', f'{input_images_dir}/frame_%04d.png',  # Input image sequence pattern
        '-c:v', 'ffv1',                         # Codec: lossless ffv1
        '-q:v', '0',                            # Quality level 0 (lossless)
        output_video                            # Output video file path
    ]
    
    # Run the FFmpeg command using subprocess
    try:
        subprocess.run(ffmpeg_command, check=True)
        print(f"Lossless video created: {output_video}")
    except subprocess.CalledProcessError as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    # Parse command-line arguments
    parser = argparse.ArgumentParser(description='Convert a file to a lossless video.')
    parser.add_argument('input_file', type=str, help='The input file to encode into video.')
    parser.add_argument('--output-path-dir', type=str, help='The directory where the output video should be saved.')

    args = parser.parse_args()

    input_file_path = args.input_file

    # Define the output directory for images
    output_images_dir = 'output_images'

    # Convert the input file to images and get the metadata
    filename, file_extension, data_length = file_to_images(input_file_path, output_images_dir)

    # Automatically generate the output video filename with the file extension and size
    video_filename = f"{filename}-{file_extension}-{data_length}.mkv"

    # Determine the full output path for the video
    if args.output_path_dir:
        # Ensure the output path exists
        if not os.path.exists(args.output_path_dir):
            os.makedirs(args.output_path_dir)
        output_video_path = os.path.join(args.output_path_dir, video_filename)
    else:
        # Use the current working directory if no output path is provided
        output_video_path = os.path.join(os.getcwd(), video_filename)

    # Create a lossless video from the image sequence
    create_lossless_video(output_images_dir, output_video_path)
