# Docker ROS Kalibr

Docker image to run Kalibr.

Built versions available on Docker Hub https://hub.docker.com/repository/docker/stereolabs/kalibr

## Usage

```
docker run -v ~/foo:/foo -it stereolabs/kalibr:kinetic
```

Example of calibration command :

```
kalibr_calibrate_cameras --bag /foo/sequence.bag --target /foo/april_6x6_80x80cm.yaml --models 'pinhole-radtan' 'pinhole-radtan' --topics /cam0/image_raw /cam1/image_raw --dont-show-report
```

## Known issue

The PDF report is not generated currently since it requires display support
