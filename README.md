# Vision Components MIPI CSI-2 driver for DIGI ConnectCore 8X Development Kit
![VC MIPI camera](https://www.vision-components.com/fileadmin/external/documentation/hardware/VC_MIPI_Camera_Module/VC_MIPI_Camera_Module_Hardware_Operating_Manual-Dateien/mipi_sensor_front_back.png)

Supported board: DIGI ConnectCore 8X Development Kit   
Supported camera: VC MIPI IMX327 / VC MIPI IMX327C   

Linux kernel version: 5.4.84   

## Version
* v0.1.0 (Pre version for demonstration only)
  * Features
    * Image streaming with faulty pixel formating
  * Known Issues
    * Exposure and Gain cannot be addressed via V4L2 library.
    * Image pixel formating does not work properly so far. This leads to an extrem noisy image.
    * The setup process in this documentation leads to some error messages while boot up.
    * Crashes in the imx8-isi driver while live image streaming.
* v0.2.0 (Image streaming with 4 bit left shifted image)
  * Features
    * Image Streaming in SRGGB10-RAW format (4 bit left shifted).
    * vcmipidemo supports software implementation to correct the 4 bit left shift.
  * Bugfixes
    * Fixed build errors of dts files.
  * Known Issues
    * Exposure and Gain cannot be addressed via V4L2 library.
    * Handling for arbitrary image width and height not ready.

## Functionality 
* Live image streaming with vcmipidemo software.
  * A headless (no display connected) hardware setup is assumed.
  * You need a Windows 10 PC to run the vcmipidemo frontend.

## Prerequisites for cross-compiling
### Host PC
* Recommended OS is Ubuntu 18.04 LTS or Ubuntu 20.04 LTS.
* You need git to clone this repository.
* All other packages are installed by the scripts contained in this repository.

# Installation
When we use the **$** sign it is meant that the command is executed on the host PC. A **#** sign indicates that you have to execute the command on the target machine.  

1. Follow the instruction from DIGI to setup your board. (DIGI original doc: [Step 2 - Set up the hardware](https://www.digi.com/resources/documentation/digidocs/embedded/dey/3.0/cc8x/yocto-gs_t_set-up-hw_8x))

2. Create a directory and clone the repository   
   ```
     $ cd <working_dir>
     $ git clone -b master https://github.com/pmliquify/vc_mipi_digi 
   ```

3. Setup the toolchain and the kernel sources.
   ```
     $ cd vc_mipi_digi/bin
     $ ./setup.sh host
   ```

4. Build the kernel image, kernel modules and device tree files.
   ```
     $ ./build.sh k
     $ ./build.sh d
   ```

5. Install a fresh BSP Image to the target
   1. Connect a USB type-C cable between target and host. 
   2. Reboot device in U-Boot console and selecting the USB interface you want it to listen to by entering ```=> fastboot 1```
   3. We provide a script to easily flash an image. It will download the tools from DIGI and start the recovery process.
      ```
        $ ./recover.sh
      ```
    (DIGI original doc: [Recover your device - Update bootloader from USB using Fastboot](
https://www.digi.com/resources/documentation/digidocs/embedded/dey/3.0/cc8x/yocto_t_recover-device_8#update-bootloader-from-usb-using-fastboot))

6. After boot up install the VC MIPI driver with the kernel image, modules and dtb files we have build in step 4.
   ```
     $ ./flash.sh a
   ```

# Testing the camera
The system should start properly. *(There are some error messages that can be ignored for this pre version)*

1. Install the vcmipidemo frontend on Windows 10. 
   1. Copy all files from *<working_dir>/src/vcmipidemo/win* to a usb stick.
   2. Install **python-2.7.11.msi** 
   3. Install **pygtk-all-in-one-2.24.2.win32-py2.7.msi**
   4. Do not start the python script yet!

2. Build and install the vcmipidemo backend on the target.
   ```
     $ ./build.sh demo
     $ ./test.sh
   ```

3. Login to the target and check if the driver was loaded properly. You should see something similar to the output in the second box.
   ```
     $ ssh root@ccimx8x-sbc-pro
     # dmesg | grep '16-00'
   ```
   ```
     [    3.455851] i2c 16-0010: VC MIPI Module - Hardware Descriptor
     [    3.461631] i2c 16-0010: [ MAGIC  ] [ mipi-module ]
     [    3.466540] i2c 16-0010: [ MANUF. ] [ Vision Components ] [ MID=0x0427 ]
     [    3.473265] i2c 16-0010: [ SENSOR ] [ SONY IMX327C ]
     [    3.478253] i2c 16-0010: [ MODULE ] [ ID=0x0327 ] [ REV=0x0002 ]
     [    3.484282] i2c 16-0010: [ MODES  ] [ NR=0x0002 ] [ BPM=0x0010 ]
     [    3.490309] i2c 16-0010: VC MIPI Sensor succesfully initialized.
     [    3.862068] mx8-img-md: Registered sensor subdevice: vc-mipi-cam 16-001a (1)
     [    3.903763] mx8-img-md: created link [vc-mipi-cam 16-001a] => [mxc-mipi-csi2.0]
   ```

3. Start the vcmipidemo to stream the image to the frontend.   
   **Please do all in the described order!**
   1. Open a new terminal, login and start the vcimgnetsrv. The server starts to listen for the connection from the VCImgNetClient.
      ```
        $ ssh root@ccimx8x-sbc-pro
        # ./test/vcimgnetsrv
        Start VC Image Net Server ...
        Listen on port 2002
      ```
   
   2. Execute the python script **vcimgnetclient.py**. 
      * The window from the VCImgNetClient should show up. 
      * Click the **Receive Image** Button.
      * In the first terminal you should see that the client had connected to the server.
        ```
          # ./test/vcimgnetsrv
          Start VC Image Net Server ...
          Listen on port 2002
          Client connected!
        ```
      * The plain gray mainframe in the VCImgNetClient window should change to a diagonal hatched pattern. 
        A moving bar shows that the app is waiting to receive image data.   
      
   3. Open a second terminal, login and start the vcmipidemo to start the image stream. The application starts streaming 
      and should show some image information and the first few bytes from the image.
      ```
        $ ssh root@ccimx8x-sbc-pro
        # ./test/vcmipidemo -a
        img.org (fmt: GREY, dx: 1920, dy: 1080, pitch: 1920) - c090 f0b0 c080 00e0 5040 5030 f000 d0f0 70d0 4030 
        img.org (fmt: GREY, dx: 1920, dy: 1080, pitch: 1920) - f060 b060 4070 5090 8050 f0a0 20b0 7030 10c0 6080 
        img.org (fmt: GREY, dx: 1920, dy: 1080, pitch: 1920) - 3060 d0a0 d020 e060 1050 7010 d090 4090 2090 7020 
        img.org (fmt: GREY, dx: 1920, dy: 1080, pitch: 1920) - a050 2010 0080 7080 b060 2010 a040 7020 9010 d050 
        img.org (fmt: GREY, dx: 1920, dy: 1080, pitch: 1920) - f050 70c0 50a0 e0c0 3060 0070 2000 d010 c0b0 90c0 
        ...
      ```
      On Windows you should see the live image stream.   
      
      ![vcmipidemo screenshot](https://raw.githubusercontent.com/pmliquify/vc_mipi_digi/main/docs/vcmipidemo_screenshot.png)
