# GooglePark
### Author: <dharmesh@tarapore.ca>


### Basics:

GooglePark Converts any file to a video, which you can then upload to YouTube, Vimeo, etc.
It's called **Google Park** because Google Drive stopped offering unlimited file storage and I'm miffed. It's also called that because I'm bad at naming things.

While this code was written in C++, [an experimental Python branch also exists](https://github.com/weirdindiankid/GooglePark/tree/experimental-python).
### Installation and Usage:

1. Install `ffmpeg` using `brew install ffmpeg` or your package manager's equivalent. Also install OpenCV using `brew install opencv` or whatever you normally use.

2. Next, run `make`. This should give you 2 executables: `file_to_video`, and `video_to_file`, respectively.

3. Then, to encode a file, `./file_to_video <path-to-input-file>`

4. To decode, `./video_to_file <path-to-file-to-decode>`. When decoding, unless you're supplying a filename which was created using `./video_to_file`, you must specify a `--file-size` in bytes and a `--file-format` so we know what to save the recovered file as.

### Demos:
Running `./file_to_video ./demos/intel_isa_manual.pdf --output-path-dir ./demos`, you can see the output video file `./demos/intel_isa_manual-pdf-11599579.mkv`, which is how encoded videos will look (i.e. a bunch of "static"):

![demooutput](https://github.com/user-attachments/assets/57b30555-d4ef-4d90-9f64-9aac2c60421b)


You can recover the original file by running `./video_to_file ./demos/intel_isa_manual-pdf-11599579.mkv` or `./video_to_file ./demos/intel_isa_manual-pdf-11599579.mkv --file-size 11599579 --file-format pdf`. The latter is redundant since we can infer the file format and size from the name, but that's how you'd invoke the `video_to_file` executable otherwise.

### Notes and Disclaimers:

1. It goes without saying that this isn't an encouragement to break YouTube's terms of service. I'm simply observing that one could theoretically get unlimited cloud storage by uploading a bunch of videos to YouTube.

2. I also don't know that YouTube won't alter the video by encoding or compressing it, should you try this. So don't use this on data you rely. If they do encode or compress the video, I suspect Reed-Solomon error correction might mitigate the issue somewhat.

3. Interestingly, this method has no issues encoding a video into a video.

4. **This does not encrypt your data so DO NOT assume it's secure!** I might add an encryption mechanism (256-bit AES shouldn't be too hard to implement before encoding), but for now, this is encoding your input file as is.

5. The largest file I've encoded during my tests is 1GB. I am not sure how reliably this will work beyond that. Please don't get mad at me if this is your primary backup strategy and you find that it doesn't work one day!

