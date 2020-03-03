# Kalibr exporting tool to ZED Configuration

Create a compatible configuration file for the ZED SDK from a result.txt kalibr file

## Usage

This program expect the Kalibr output calibraiton file and the ZED camera serial number

For instance :


```bash
# Compilation
mkdir build
cd build ; cmake .. ; make

# Launch
./export_kalibr_to_zed results-cam-<rosbag_path>rosbagfilename.txt 22580486
```

It will create ZED compatible conf file `SN22580486.conf`, copy it to `/usr/local/zed/settings`
