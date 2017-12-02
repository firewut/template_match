# About

C++ template matching extractor.

# Crontab

Every day download latest streams from popular streamers using `youtube-dl`.

Then call

```
  extractor <path_to_video_file> <path_to_image_file> <seconds_before_appear> <seconds_after_appear> <max_templates_allowed> <debug>

  extractor /var/files/video.mp4 /var/files/concede.png 5 2 1 0
```
