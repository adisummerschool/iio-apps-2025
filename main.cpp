#include "matplotlibcpp.h"
#include <iostream>
#include <iio.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <vector>

#define URI "ip:10.76.84.239"
#define DEV_NAME "ad5592r_s"
#define SAMPLE_COUNT 32
#define THRESHOLD 100
#define SAMPLE_FREQUENCY 50

namespace plt = matplotlibcpp;

typedef struct {
        uint16_t xpos;
        uint16_t xneg;
        uint16_t ypos;
        uint16_t yneg;
        uint16_t zpos;
        uint16_t zneg;
} current_samples;

int main(int arc, char **argv) {
        struct iio_context* ctx = iio_create_context_from_uri(URI);
        if (!ctx) {
                printf("Context Error: %s\n", strerror(errno));
        }

        struct iio_device* trigger = iio_context_find_device(ctx, "trigger0");
        if(!trigger) {
                printf("Did not find the trigger\n");
                return 1;
        }

        double read_frequency;
        int check;
        int check_write;
        check = iio_device_attr_read_double(trigger, "sampling_frequency", &read_frequency);
        if(check < 0) {
                printf("Error occured at reading sampling frequency!\n");
                return -1;
        }
        else {
                if (read_frequency != SAMPLE_FREQUENCY){
                        check_write = iio_device_attr_write_double(trigger, "sampling_frequency", SAMPLE_FREQUENCY);
                        if(check_write < 0){
                                printf("Error occured at writing sampling frequency!\n");
                                return -1;
                        }
                }
        }

        struct iio_device* dev = iio_context_find_device(ctx, DEV_NAME);
        if(!dev) {
                printf("Device Name Error\n");
                return -1;
        }

        struct iio_channel *channels[6];
        const char* channel_names[] = {"voltage0", "voltage1", "voltage2", "voltage3", "voltage4", "voltage5"};
        for(int i=0; i<6; i++) {
                channels[i] = iio_device_find_channel(dev, channel_names[i], false);
                if (!channels[i]) {
                        printf("Something went wrong!\n");
                        return -1;
                }
                iio_channel_enable(channels[i]);
        }

        iio_buffer *buf = iio_device_create_buffer(dev, SAMPLE_COUNT, false);
        if (!buf) {
                printf("Could not create buffer, errno: %d\n", errno);
                return -errno;
        }

        ssize_t bytes_read = iio_buffer_refill(buf);
        if (bytes_read < 0) {
                printf("Could not refill buffer, errno: %d\n", errno);
                return -errno;
        }

        std::vector<int> x_axis(SAMPLE_COUNT), y_axis(SAMPLE_COUNT), z_axis(SAMPLE_COUNT), t_axis(SAMPLE_COUNT);
        uint16_t step_size = iio_buffer_step(buf);
        current_samples current_sample;
        int cnt = 0;
        for (uint16_t *sample = static_cast<uint16_t *>(iio_buffer_start(buf)); sample < iio_buffer_end(buf); sample += step_size) {
                memcpy(&current_sample, sample, 6 * sizeof(uint16_t));
                //current_sample.xpos = *sample;
                //current_sample.xneg = *(sample + sizeof(uint16_t));
                //current_sample.ypos = *(sample + 2 * sizeof(uint16_t));
                //current_sample.yneg = *(sample + 3 * sizeof(uint16_t));
                //current_sample.zpos = *(sample + 4 * sizeof(uint16_t));
                //current_sample.zneg = *(sample + 5 * sizeof(uint16_t));

                std::cout <<"x_pos:"<< current_sample.xpos << "            x_neg:" << current_sample.xneg << "\n"
                          <<"y_pos:"<< current_sample.ypos << "            y_neg:" << current_sample.yneg << "\n"
                          <<"z_pos:"<< current_sample.zpos << "            z_neg:" << current_sample.zneg << "\n";

                x_axis.push_back(current_sample.xpos - current_sample.xneg);
                y_axis.push_back(current_sample.ypos - current_sample.yneg);
                z_axis.push_back(current_sample.zpos - current_sample.zneg);
                t_axis.push_back(cnt++);
        }

        // for (int i = 1; i < x_axis.size(); i++) {
        //         int x_diff = abs(x_axis[i] - x_axis[i-1]);
        //         if (x_diff >= THRESHOLD) {
        //                 std::cout << "X axis exceeded threshold\n";
        //         }

        //         int y_diff = abs(y_axis[i] - y_axis[i-1]);
        //         if (y_diff >= THRESHOLD) {
        //                 std::cout << "Y axis exceeded threshold\n";
        //         }

        //         int z_diff = abs(z_axis[i] - z_axis[i-1]);
        //         if (z_diff >= THRESHOLD) {
        //                 std::cout << "Z axis exceeded threshold\n";
        //         }
        // }

        plt::figure_size(1000, 600);
        plt::named_plot("X", t_axis, x_axis, "b-");
        plt::named_plot("Y", t_axis, y_axis, "r-");
        plt::named_plot("Z", t_axis, z_axis, "g-");
        plt::xlabel("Sample");
        plt::ylabel("Acc");
        plt::legend();
        plt::show();

        printf("All good! Congratulations!\n");
        return 0;
}