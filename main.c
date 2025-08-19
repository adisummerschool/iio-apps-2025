#include <stdio.h>
#include <iio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include "print.h"

#define THRESHHOLD      80

int main(int argc, char **argv) {
        /* Check arguments */
        if (argc < 4) {
                printf("Not enough arguments!\n");
                return -1;
        }

        char *uri = argv[1];
        char *device_name = argv[2];
        char *axis = argv[3];

        /* Create IIO context */
        struct iio_context *ctx = iio_create_context_from_uri(uri);
        if (!ctx) {
                printf("No context found, error: %f\n", strerror(errno));
                return -errno;
        }

        /* Find the IIO device */
        struct iio_device *dev = iio_context_find_device(ctx, device_name);
        if (!dev) {
                printf("No device found, error: %s\n");
                return -1;
        }

        /* Find the IIO channel */
        struct iio_channel *channel0 = iio_device_find_channel(dev, "voltage0", false);
        if (!channel0) {
                printf("No voltage0 channel found, error: %p\n", channel0);
                return -1;
        }

        struct iio_channel *channel1 = iio_device_find_channel(dev, "voltage1", false);
        if (!channel1) {                
                printf("No voltage1 channel found, error: %p\n", channel1);
                return -1;
        }

        struct iio_channel *channel2 = iio_device_find_channel(dev, "voltage2", false);
        if (!channel2) {
                printf("No voltage2 channel found, error: %p\n", channel2);
                return -1;
        }

        struct iio_channel *channel3 = iio_device_find_channel(dev, "voltage3", false);
        if (!channel3) {
                printf("No voltage3 channel found, error: %p\n", channel3);
                return -1;
        }

        struct iio_channel *channel4 = iio_device_find_channel(dev, "voltage4", false);
        if (!channel4) {
                printf("No voltage4 channel found, error: %p\n", channel4);
                return -1;
        }

        struct iio_channel *channel5 = iio_device_find_channel(dev, "voltage5", false);
        if (!channel5) {
                printf("No voltage5 channel found, error: %p\n", channel5);
                return -1;
        }

        /* Calibrate */
        double voltage_raw0 = -1;
        double voltage_raw1 = -1;
        while(true)
        {
                int res0 = iio_channel_attr_read_double(channel0, "raw", &voltage_raw0);
                int res1 = iio_channel_attr_read_double(channel1, "raw", &voltage_raw1);

                printf("Voltage 0: %f, Voltage 1: %f\n", voltage_raw0, voltage_raw1);
                if (voltage_raw0 > THRESHHOLD)
                        printf("Spin left!\n");
                else if (voltage_raw1 > THRESHHOLD) {
                        printf("Spin right!\n");
                } else {
                        printf("OK!\n");
                }

                usleep(100000);
        }

        /* Determine channels based on axis argument */
        struct iio_channel *minus_channel = NULL;
        struct iio_channel *plus_channel = NULL;

        if (strcmp(axis, "x") == 0) {
                minus_channel = channel0;
                plus_channel = channel1;
        } else if (strcmp(axis, "y") == 0) {
                minus_channel = channel2;
                plus_channel = channel3;
        } else if (strcmp(axis, "z") == 0) {
                minus_channel = channel4;
                plus_channel = channel5;
        } else {
                printf("Invalid axis argument! Use x, y, or z.\n");
                iio_context_destroy(ctx);
                return -1;
        }

        /* Read voltage channels for selected axis */
        double axis_voltage_minus = -1;
        double axis_voltage_plus = -1;
        int res_minus = iio_channel_attr_read_double(minus_channel, "raw", &axis_voltage_minus);
        int res_plus = iio_channel_attr_read_double(plus_channel, "raw", &axis_voltage_plus);

        if (res_minus < 0 || res_plus < 0) {
                printf("Error reading axis voltages!\n");
        } else {
                printf("Axis %s voltages: minus=%f, plus=%f\n", axis, axis_voltage_minus, axis_voltage_plus);
        }

        iio_context_destroy(ctx);
        return 0;
}
