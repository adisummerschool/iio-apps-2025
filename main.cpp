#include <stdio.h>
#include <iostream>
#include <iio.h>
#include <errno.h>
#include <string.h>
#include <vector>
#include "matplotlibcpp.h"

#define URI "ip:10.76.84.137"
#define DEV_NAME "iio-adc-ad5592"
#define SAMPLE_COUNT 32
#define THR 100
#define SAMPLING_FREQ 50

namespace plt = matplotlibcpp;

typedef struct {
        uint16_t xpos, xneg, ypos, yneg, zpos, zneg;
} current_samples;

int main(int argc, char** argv){
        struct iio_context* ctx = iio_create_context_from_uri(URI);
        if (!ctx){
                printf("context not work: %s\n", strerror(errno));
                return -errno;
        }

        //modify sample rate
        struct iio_device *trigger = iio_context_find_device(ctx, "trigger0");
        if (!trigger){
                printf("could not find trigger\n");
                return -1;
        }

        double freq;
        int ret = iio_device_attr_read_double(trigger,"sampling_frequency",&freq);
        if (ret != 0){
                printf("could not read from trigger");
                return -10;
        }
        if (freq != SAMPLING_FREQ){
               ret = iio_device_attr_write_double(trigger,"sampling_frequency",freq); 
                if (ret != 0){
                printf("could not write to trigger");
                return -10;
        }
        }


        struct iio_device* dev = iio_context_find_device(ctx, argv[2]);
        if (!dev){
                printf("device name not correct\n");
                return -1;
        }

        struct iio_channel *channels[6];
        for (int i=0; i<6; ++i){
                char c[2]="";
                c[1]=0;
                c[0] = i + '0';
                channels[i] = iio_device_find_channel(dev, strcat("voltage", c), false); //false = input, true = output
                if (!channels[i]){
                        printf("channel error\n");
                        return -2;
                }
                iio_channel_enable(channels[i]);
        }

        iio_buffer *buf = iio_device_create_buffer(dev, SAMPLE_COUNT, false);
        if (!buf){
                std::cerr<<"Could not create buffer, errno: "<<errno<< '\n';
                return -4;
        }
        ssize_t bytes_read = iio_buffer_refill(buf);
        if (bytes_read < 0){
                std::cerr<<"Could not refill buffer, errno: "<<errno<<'\n';
        }
        int i=0;
        ptrdiff_t step_size = iio_buffer_step(buf);
        std::vector<int32_t> x_axis, y_axis, z_axis, t_axis;
        for (uint16_t *sample = static_cast<uint16_t *>(iio_buffer_start(buf)); sample < iio_buffer_end(buf); sample += step_size){
                current_samples current_sample;
                memcpy(&current_sample, sample, sizeof(current_sample));
                std::cout<<current_sample.xpos<<' '<<current_sample.xneg<<' '<<current_sample.ypos<<' '<<current_sample.yneg<<' '<<current_sample.zpos<<' '<<current_sample.zneg<<'\n';
                x_axis.push_back(current_sample.xpos - current_sample.xneg);
                y_axis.push_back(current_sample.ypos - current_sample.yneg);
                z_axis.push_back(current_sample.zpos - current_sample.zneg);
                t_axis.push_back(i++);
        }

        // for (size_t i = 1; i<x_axis.size(); ++i){
        //         int32_t x_diff = abs(x_axis[i] - x_axis[i-1]);
        //         int32_t y_diff = abs(y_axis[i] - y_axis[i-1]);
        //         int32_t z_diff = abs(z_axis[i] - z_axis[i-1]);
        //         if (x_diff > THR){
        //                 printf("X Threshold Exceeded");
        //         }
        //         if (y_diff > THR){
        //                 printf("Y Threshold Exceeded");
        //         }
        //         if (z_diff > THR){
        //                 printf("Z Threshold Exceeded");
        //         }
        // }

        plt::figure_size(1000,600);
        plt::named_plot("X", t_axis, x_axis, "b-");
        plt::named_plot("Y", t_axis, y_axis, "r-");
        plt::named_plot("Z", t_axis, z_axis, "g-");
        plt::xlabel("Samples");
        plt::ylabel("Acc");
        plt::legend();
        plt::show();
        return 0;
}