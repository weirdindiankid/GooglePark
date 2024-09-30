# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 `pkg-config --cflags opencv4`
LDFLAGS = `pkg-config --libs opencv4`

# Executable names
EXE1 = file_to_video
EXE2 = video_to_file

# Source files
SRC1 = encode.cpp
SRC2 = decode.cpp

# Default target
all: check_dependencies $(EXE1) $(EXE2)

# Compile file_to_video
$(EXE1): $(SRC1)
	$(CXX) $(CXXFLAGS) -o $(EXE1) $(SRC1) $(LDFLAGS)

# Compile video_to_file
$(EXE2): $(SRC2)
	$(CXX) $(CXXFLAGS) -o $(EXE2) $(SRC2) $(LDFLAGS)

# Dependency check
check_dependencies:
	@command -v pkg-config >/dev/null 2>&1 || { echo >&2 "pkg-config is required but not installed. Aborting."; exit 1; }
	@pkg-config --exists opencv4 || { echo >&2 "OpenCV is not installed or not found. Please install it using 'brew install opencv'. Aborting."; exit 1; }
	@command -v ffmpeg >/dev/null 2>&1 || { echo >&2 "FFmpeg is required but not installed. Please install it using 'brew install ffmpeg'. Aborting."; exit 1; }

# Clean up the build
clean:
	rm -f $(EXE1) $(EXE2)
