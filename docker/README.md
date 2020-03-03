# Docker ROS Kalibr

Docker image to run Kalibr.

Built versions available on Docker Hub https://hub.docker.com/repository/docker/stereolabs/kalibr

## Usage

### With PDF report

First enable the display authorization. This method is simple but not safe, see the [ROS Docker doc](http://wiki.ros.org/docker/Tutorials/GUI) for more information

```
xhost +local:root
```

```
docker run -it -e "DISPLAY" -e "QT_X11_NO_MITSHM=1" -v "/tmp/.X11-unix:/tmp/.X11-unix:rw" -v "~/foo:/foo" stereolabs/kalibr:kinetic
```

Example of calibration command :

```
kalibr_calibrate_cameras --bag /foo/sequence.bag --target /foo/april_6x6_80x80cm.yaml --models 'pinhole-radtan' 'pinhole-radtan' --topics /cam0/image_raw /cam1/image_raw
```


### Without display

```
docker run -v ~/foo:/foo -it stereolabs/kalibr:kinetic
```

Example of calibration command :

```
kalibr_calibrate_cameras --bag /foo/sequence.bag --target /foo/april_6x6_80x80cm.yaml --models 'pinhole-radtan' 'pinhole-radtan' --topics /cam0/image_raw /cam1/image_raw --dont-show-report
```
