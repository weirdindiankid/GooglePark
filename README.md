# GooglePark
### Author: <dharmesh@tarapore.ca>


### Basics

GooglePark Converts any file to a video, which you can then upload to YouTube, Vimeo, etc.
It's called **Google Park** because Google Drive stopped offering unlimited file storage and I'm miffed. It's also called that because I'm bad at naming things.

### Installation and Usage

1. Install `ffmpeg` using `brew install ffmpeg` or your package manager's equivalent.

2. Next, create a virtual environment using `virtualenv venv`

3. Activate your virtual environment and install requirements: `source venv/bin/activate && pip install -r requirements.txt`

4. Then, to encode a file, `python encode.py <path-to-input-file>`

5. To decode, `python decode.py <path-to-file-to-decode>`. When decoding, unless you're supplying a filename which was created using `encode.py`, you must specify a `--file-size` in bytes and a `--file-format` so we know what to save the recovered file as.

### Demos
Running `python encode.py ./demos/intel_isa_manual.pdf --output-path-dir ./demos`, you can see the output video file `./demos/intel_isa_manual-pdf-11599579.mkv`, which is how encoded videos will look (i.e. a bunch of "static").

You can recover the original file by running `python decode.py ./demos/intel_isa_manual-pdf-11599579.mkv` or `python decode.py ./demos/intel_isa_manual-pdf-11599579.mkv --file-size 11599579 --file-format pdf`. The latter is redundant since we can infer the file format and size from the name, but that's how you'd invoke the decode script otherwise.

### Notes and Disclaimers

1. It goes without saying that this isn't an encouragement to break YouTube's terms of service. I'm simply observing that one could theoretically get unlimited cloud storage by uploading a bunch of videos to YouTube.

2. I also don't know that YouTube won't alter the video by encoding or compressing it, should you try this. So don't use this on data you rely.

3. Interestingly, this method has no issues encoding a video into a video.

4. **This does not encrypt your data so DO NOT assume it's secure!** I might add an encryption mechanism (256-bit AES shouldn't be too hard to implement before encoding), but for now, this is encoding your input file as is.

5. The largest file I've encoded during my tests is 1GB. I am not sure how reliably this will work beyond that. Please don't get mad at me if this is your primary backup strategy and you find that it doesn't work one day!

