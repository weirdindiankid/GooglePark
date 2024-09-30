import cv2
import numpy as np
import os
import shutil
import subprocess
import argparse

def clear_or_create_dir(directory):
    """Empty the directory if it exists, otherwise create it."""
    if os.path.exists(directory):
        # Remove all contents in the directory
        shutil.rmtree(directory)
    # Recreate the directory
    os.makedirs(directory)

def extract_frames_from_video(video_file, output_dir):
    """Run FFmpeg to extract frames from video."""
    # Clear or create the output directory
    clear_or_create_dir(output_dir)

    # FFmpeg command to extract frames from video
    ffmpeg_command = [
        'ffmpeg',
        '-i', video_file,              # Input video file
        '-vsync', '0',                 # Disable frame rate synchronization
        f'{output_dir}/frame_%04d.png' # Output file pattern
    ]

    # Run the FFmpeg command using subprocess
    try:
        subprocess.run(ffmpeg_command, check=True)
        print(f"Frames extracted to: {output_dir}")
    except subprocess.CalledProcessError as e:
        print(f"Error: {e}")

def infer_file_details_from_filename(video_filename):
    """
    Infer the original file's name, format, and size from the video filename.
    The filename format should be: filename-extension-size.mkv
    """
    # Extract the base filename without the extension
    base_name = os.path.basename(video_filename)
    name_parts = base_name.rsplit('-', 2)  # Split based on the last two dashes

    if len(name_parts) != 3 or not name_parts[2].endswith('.mkv'):
        raise ValueError(f"Filename {base_name} is not in the expected format: 'filename-extension-size.mkv'")

    # Extract filename, extension, and size
    filename = name_parts[0]
    file_extension = name_parts[1]
    file_size = int(name_parts[2].replace('.mkv', ''))  # Remove the '.mkv' part

    # Reconstruct the original file name with the extension
    original_filename = f"recovered_{filename}.{file_extension}"
    
    return original_filename, file_size

def images_to_file(input_dir, output_file, frame_size=(640, 480), original_length=None):
    """Convert the extracted frames back into the original file."""
    decoded_data = []

    # List all image files in the directory
    image_files = sorted([f for f in os.listdir(input_dir) if f.endswith('.png')])

    for image_file in image_files:
        # Read each frame as a grayscale image
        frame = cv2.imread(os.path.join(input_dir, image_file), cv2.IMREAD_GRAYSCALE)
        decoded_data.append(frame.flatten())

    # Concatenate the data and trim to the original length
    decoded_data = np.concatenate(decoded_data, axis=0)
    byte_data = decoded_data[:original_length].astype(np.uint8).tobytes()

    # Write the byte data back to the file
    with open(output_file, 'wb') as f:
        f.write(byte_data)

    print(f"Recovered file saved as {output_file}")

if __name__ == "__main__":
    # Parse command-line arguments
    parser = argparse.ArgumentParser(description='Recover original file from a video.')
    parser.add_argument('video_file', type=str, help='The video file to decode.')
    parser.add_argument('--file-size', type=int, help='The original file size in bytes.')
    parser.add_argument('--file-format', type=str, help='The original file format (e.g., zip, pdf).')

    args = parser.parse_args()

    # Step 1: If --file-size and --file-format are provided, use them; otherwise, try to infer
    if args.file_size and args.file_format:
        original_filename = f"recovered_file.{args.file_format}"
        original_size = args.file_size
        print(f"Using provided size and format: {original_filename}, Size: {original_size} bytes")
    else:
        try:
            original_filename, original_size = infer_file_details_from_filename(args.video_file)
            print(f"Inferred original file: {original_filename}, Size: {original_size} bytes")
        except ValueError as e:
            # If we can't infer the details and no size or format is provided, output an error
            print(e)
            print("Error: Please provide --file-size and --file-format or rename the input file to follow the format 'filename-extension-size.mkv'.")
            exit(1)

    # Step 2: Extract frames from the video
    output_images_dir = 'decoded_images'
    extract_frames_from_video(args.video_file, output_images_dir)

    # Step 3: Reconstruct the original file from the frames
    images_to_file(output_images_dir, original_filename, frame_size=(640, 480), original_length=original_size)
