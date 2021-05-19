#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <linux/v4l2-subdev.h>


int set_gain(int fd, int value)
{
        struct v4l2_control control;
        control.id = V4L2_CID_GAIN;
        control.value = value;
        return ioctl(fd, VIDIOC_S_CTRL, &control);
}

int set_exposure(int fd, int value)
{
        struct v4l2_control control;
        control.id = V4L2_CID_EXPOSURE;
        control.value = value;
        return ioctl(fd, VIDIOC_S_CTRL, &control);
}

int set_format(int fd)
{
	struct v4l2_format format;
	format.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_SRGGB10;
	format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	format.fmt.pix.width = 1920;
	format.fmt.pix.height = 1080;
	return ioctl(fd, VIDIOC_S_FMT, &format);
}

int  main(int argc, char *argv[])
{
        char *device = "/dev/video0"; 

        printf("Open %s\n", device);
        int fd = open(device, O_RDWR, 0);
        if(fd == -1) {
                printf("Could not open %s\n", device);
        }
        
        set_gain(fd, 100);
        set_exposure(fd, 20000);
        set_format(fd);
}