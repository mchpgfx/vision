---
parent: Vision Example Applications
title: Libcamera Display
nav_order: 1
---

# Camera Demo Applications

This application captures video frames directly from the image sensor module through either a MIPI CSI-2 or a Parallel interface. It then displays the acquired frames on the screen using the following development boards.

|MPLABX Configuration|Board Configuration|
|:-------------------|:------------------|
|[libcamera_lvds_sam9x75_curiosity.X](./firmware/libcamera_lvds_sam9x75_curiosity.X/readme.md)| [SAM9X75-Curiosity Board](https://www.microchip.com/en-us/development-tool/EA14J50A) using the MIPI CSI interface to capture video frames from the [Sony IMX219 Camera Module](https://www.raspberrypi.com/products/camera-module-v2/) and display it on to the [AC69T88A LVDS display](https://www.microchip.com/en-us/development-tool/AC69T88A) |
|[libcamera_lvds_sam9x75_eb.X](./firmware/libcamera_lvds_sam9x75_eb.X/readme.md)| [SAM9X75-DDR3-EB Early Access Evaluation Board](https://www.microchip.com/en-us/development-tool/EA14J50A) using the MIPI CSI interface to capture video frames from the [Sony IMX219 Camera Module](https://www.raspberrypi.com/products/camera-module-v2/) and display it on to the [AC69T88A LVDS display](https://www.microchip.com/en-us/development-tool/AC69T88A) |
|[libcamera_sama5d27_som1_ek1.X](./firmware/libcamera_sama5d27_som1_ek1.X/readme.md)| [SAMA5D27-SOM1-EK1](https://www.microchip.com/en-us/development-tool/ATSAMA5D27-SOM1-EK1) using Parallel interface to capture video frames from either [OV5640 Camera Board](https://www.waveshare.com/ov5640-camera-board-a.htm) or [OV2640 Camera Board](https://www.waveshare.com/ov2640-camera-board.htm) and display it on to the PDA TM5000 display |


