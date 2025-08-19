#include <iostream>
#include <iio.h>
#include <errno.h>
#include <string.h>
#include <vector>
#include "matplotlibcpp.h"

namespace plt = matplotlibcpp;

#define URI     "ip:10.76.84.222"
#define DEV_NAME        "iio-ad5592"
#define SAMPLE_COUNT    32
#define THRESHOLD       100
#define SAMPLE_FREQUENCY 50

typedef struct {
        uint16_t xpos;
        uint16_t xneg;

        uint16_t ypos;
        uint16_t yneg;
        
        uint16_t zpos;
        uint16_t zneg;
} current_samples;


int main(int argc, char **argv) {
        
        struct iio_context* ctx;
        struct iio_device* dev;
        struct iio_channel *channels[6];
        struct iio_buffer *buf;
        int i;
        int ret;
        double val[6];
        double freq;
        std::vector<int> x_axis, y_axis, z_axis, t_axis;
        std::string channelName;
        ssize_t refill;
        ssize_t frequency_err;
        
        
        ctx = iio_create_context_from_uri(URI);
        if (!ctx) {
                printf("Context does not work. Error: %s\n", strerror(errno));
                return -errno;
        }

        struct iio_device *trigger = iio_context_find_device(ctx, "trigger0");
        if (!trigger) {
                printf("The device name is wrong\n");
                return -1;
        }

        ret = iio_device_attr_read_double(trigger, "sampling_frequency", &freq);
        if (ret) {
                printf("Ai gresit ceva la citire, eroare: %d\n", ret);
                return ret;
        }
        if(freq != SAMPLE_FREQUENCY) {
                frequency_err =  iio_device_attr_write_double(trigger, "sampling_frequency", SAMPLE_FREQUENCY);
                if(frequency_err < 0 ) {
                        printf("Writing sampling frequency did not work. Error: %s\n", strerror(errno));
                        return -errno;
                }
        }
        // ret = iio_device_attr_read_double(trigger, "sampling_frequency", &freq);
        // if (ret) {
        //         printf("Ai gresit ceva la citire, eroare: %d\n", ret);
        //         return ret;
        // }
        // printf("Frecventa noua: %.0f\n", freq);

        dev = iio_context_find_device(ctx, DEV_NAME);
        if (!dev) {
                printf("The device name is wrong\n");
                return -1;
        }
        
        
        for (i=0; i<6; i++) {
                channelName = "voltage" + std::to_string(i);
                channels[i] = iio_device_find_channel(dev, channelName.c_str(), false);
                if (!channels[i]) {
                        printf("Something went wrong finding the channels\n");
                        return -1;
                }
        }
        
        for (i=0; i<6; i++) {
                ret = iio_channel_attr_read_double(channels[i], "raw", &val[i]);
                if (ret) {
                        printf("Ai gresit ceva la citire, eroare: %d\n", ret);
                        return ret;
                }
                iio_channel_enable(channels[i]);
        }

        // printf("xpos: %.0f\txneg: %.0f\typos: %.0f\tyneg: %.0f\tzpos: %.0f\tzneg: %.0f\n", val[0],val[1],val[2],val[3],val[4],val[5]);

        buf = iio_device_create_buffer(dev, SAMPLE_COUNT, false);
        if(!buf) {
                printf("Buffer creation does not work. Error: %s\n", strerror(errno));
                return -errno;
        }

        refill = iio_buffer_refill(buf);
        if(refill < 0 ) {
                printf("Buffer refill does not work. Error: %s\n", strerror(errno));
                return -errno;
        }
        else {
              printf("Number of bytes: %li\n", refill);  
        }

        ptrdiff_t step_size =  iio_buffer_step(buf)/sizeof(uint16_t);
        printf("%li\n",step_size);
        int count = 0;
        for (uint16_t *sample = static_cast<uint16_t *>(iio_buffer_start(buf)); sample < iio_buffer_end(buf); 
                sample += step_size)  {
                // current_samples current_sample;
                // current_sample.xpos = *sample;
                // current_sample.xneg = *(sample + 1);
                // current_sample.ypos = *(sample + 2);
                // current_sample.yneg = *(sample + 3);
                // current_sample.zpos = *(sample + 4);
                // current_sample.zneg = *(sample + 5);

                current_samples current_sample;
                memcpy(&current_sample, sample, sizeof(current_sample));
                
                x_axis.push_back(current_sample.xpos - current_sample.xneg);
                y_axis.push_back(current_sample.ypos - current_sample.yneg);
                z_axis.push_back(current_sample.zpos - current_sample.zneg);
                t_axis.push_back(count++);

                printf("%d\t%d\t%d\t%d\t%d\t%d\n", 
                current_sample.xpos,
                current_sample.xneg,
                current_sample.ypos,
                current_sample.yneg,
                current_sample.zpos,
                current_sample.zneg);
        }

        // printf("SIZE: %d\n",x_axis.size());

        // for (i = 1; i < x_axis.size(); ++i) {
             
        //         int x_diff = abs(x_axis[i] - x_axis[i-1]);
        //         if (x_diff >= THRESHOLD) {
        //                 printf("X axis exceeded threshold\n");
        //         }
        //         int y_diff = abs(y_axis[i] - y_axis[i-1]);
        //         if (y_diff >= THRESHOLD) {
        //                 printf("Y axis exceeded threshold\n");
        //         }
        //         int z_diff = abs(z_axis[i] - z_axis[i-1]);
        //         if (z_diff >= THRESHOLD) {
        //                 printf("Z axis exceeded threshold\n");
        //         }
        //         // printf("%d: \t%d\t%d\t%d\n", i, x_diff, y_diff, z_diff);
        // }

        plt::figure_size(1000, 600);
        plt::named_plot("X", t_axis, x_axis, "b-");
        plt::named_plot("Y", t_axis, y_axis, "r-");
        plt::named_plot("Z", t_axis, z_axis, "g-");
        plt::xlabel("Samples");
        plt::ylabel("Acc");
        plt::legend();
        plt::show();

        iio_buffer_destroy(buf);
        iio_context_destroy(ctx);
        return 0;
} 