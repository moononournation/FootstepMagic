# Footstep Magic

## convert AVI

```console
ffmpeg -y -ss 0:52.29 -t 2 -i "Why are snowflakes like this？ [ao2Jfm35XeE].webm" -vf "rotate=0.110,crop=1936:1680:942:230,tpad=stop_mode=clone:stop_duration=8" out1r.mp4
ffmpeg -y -i out1r.mp4 -vf "perspective=-W*.1:0:W*1.1:0:W*.1:H:W*0.9:H,setpts=0.2*PTS" out1p.mp4
ffmpeg -y -t 0:0.44 -i out1p.mp4 -ac 2 -ar 44100 -af loudnorm -c:a mp3 -c:v cinepak -q:v 10 -vf "fps=25,eq=brightness=0.4:contrast=1.9:saturation=2,scale=100:88" cinepak100x88.avi
ffmpeg -y -t 0:0.44 -i out1p.mp4 -ac 2 -ar 44100 -af loudnorm -c:a mp3 -c:v mjpeg -q:v 10 -vf "fps=25,eq=brightness=0.4:contrast=1.9:saturation=2,scale=100:88" mjpeg100x88.avi
```
