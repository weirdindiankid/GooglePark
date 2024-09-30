# GooglePark
## Author: <dharmesh@tarapore.pa>


### Basics

GooglePark Converts any file to a video, which you can then upload to YouTube, Vimeo, etc.
It's called **Google Park** because Google Drive stopped offering unlimited file storage and I'm miffed. It's also called that because I'm bad at naming things.

### Installation and Usage

1. Install `ffmpeg` using `brew install ffmpeg` or your package manager's equivalent.

2. Next, create a virtual environment using `virtualenv venv`

3. Activate your virtual environment and install requirements: `source venv/bin/activate && pip install -r requirements.txt`

4. Then, to encode a file, `python encode.py <path-to-input-file>`

5. To decode, `python decode.py <path-to-file-to-decode>`

### Notes and Disclaimers

1. It goes without saying that this isn't an encouragement to break YouTube's terms of service. I'm simply observing that one could theoretically get unlimited cloud storage by uploading a bunch of videos to YouTube.

2. I also don't know that YouTube won't alter the video by encoding or compressing it, should you try this. So don't use this on data you rely.

3. Interestingly, this method has no issues encoding a video into a video.

4. **This does not encrypt your data so DO NOT assume it's secure!** I might add an encryption mechanism (256-bit AES shouldn't be too hard to implement before encoding), but for now, this is encoding your input file as is.
