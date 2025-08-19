#include <iostream>
#include <iio.h>
#include <errno.h>
#include <string.h>
#include <vector>
#include "matplotlibcpp.h"

namespace plt = matplotlibcpp;

#define URI "ip:10.76.84.238"
#define DEV_NAME "iio-adc-ad5592"
#define SAMPLE_COUNT 50
#define THRESHOLD 500
#define SAMPLE_FREQUENCY 50

typedef struct {
        uint16_t xpos;
        uint16_t xneg;

        uint16_t ypos;
        uint16_t yneg;

        uint16_t zpos;
        uint16_t zneg;
} current_samples;

const char *attributes[] = {
        "voltage0",
        "voltage1",
        "voltage2",
        "voltage3",
        "voltage4",
        "voltage5"
    };

std::vector<int> x_axis, y_axis, z_axis, t_axis;
int ret;

int main(int argc, char **argv){
        struct iio_context* ctx = iio_create_context_from_uri(URI);
        if(!ctx){
                printf("Failed at creating context: %s\n",
                        strerror(errno));
                return -errno;
        }

        /// Adjust the device sample frequency
        struct iio_device* trigger = iio_context_find_device(ctx, "trigger0");
        if(!trigger){
                printf("Failed at finding trigger\n");
                return -1;
        }

        double frequency;
        ret = iio_device_attr_read_double(trigger, "sampling_frequency", &frequency);
        std::cout<< "Current frequency is " << frequency << "\n";
        if((int)frequency != SAMPLE_FREQUENCY){
                ret = iio_device_attr_write_double(trigger, "sampling_frequency", SAMPLE_FREQUENCY);
                if(!ret){
                       std::cout<< "Frequency set from " << frequency << " to " << SAMPLE_FREQUENCY << "\n";  
                }
        }
        

        struct iio_device* dev = iio_context_find_device(ctx, DEV_NAME);
        if(!dev){
                printf("Failed at finding device\n");
                return -1;
        }

        struct iio_channel *channels[6];
        for(int i=0;i<6;i++){
                channels[i] = iio_device_find_channel(dev, attributes[i], false);
                if(!channels[i]){
                        printf("Finding channel %d failed\n", i);
                        return -1;
                }
                iio_channel_enable(channels[i]);
        }
        
        iio_buffer *buf = iio_device_create_buffer(dev, SAMPLE_COUNT, false);
        if(!buf){
                printf("Failed at creating buffer: %s\n",
                        strerror(errno));
                return -1;
        }

        ssize_t bytes_read = iio_buffer_refill(buf);
        if(bytes_read < 0){
                std::cerr << " Could not refill buffer, errno: " << errno << std::endl;
        }

        int count = 0;
        current_samples current_sample;
        ptrdiff_t step_size = iio_buffer_step(buf);
        for(uint16_t *sample = static_cast<uint16_t*>(iio_buffer_start(buf)); 
                        sample < iio_buffer_end(buf); sample += step_size){
                memcpy(&current_sample, sample, sizeof(current_sample));

                std::cout << current_sample.xpos << " " << current_sample.xneg << " "
                          << current_sample.ypos << " " << current_sample.yneg << " "
                          << current_sample.zpos << " " << current_sample.zneg << '\n';

                x_axis.push_back(current_sample.xpos - current_sample.xneg);
                y_axis.push_back(current_sample.ypos - current_sample.yneg);
                z_axis.push_back(current_sample.zpos - current_sample.zneg);
                t_axis.push_back(count++);
        }

        // for(size_t i = 1; i < x_axis.size(); i++){
        //         int x_diff = abs(x_axis[i] - x_axis[i-1]);
        //         if(x_diff >= THRESHOLD){
        //                 std::cout<< "X axis exceeded threshold\n";
        //         }

        //         int y_diff = abs(y_axis[i] - y_axis[i-1]);
        //         if(y_diff >= THRESHOLD){
        //                 std::cout<< "Y axis exceeded threshold\n";
        //         }

        //         int z_diff = abs(z_axis[i] - z_axis[i-1]);
        //         if(z_diff >= THRESHOLD){
        //                 std::cout<< "Z axis exceeded threshold\n";
        //         }
        // }
        //printf("%d %d %d %d\n", x_axis.size(), y_axis.size(), z_axis.size(), t_axis.size());
        plt::figure_size(1000, 600);
        plt::named_plot("X", t_axis, x_axis, "b-");
        plt::named_plot("Y", t_axis, y_axis, "r-");
        plt::named_plot("Z", t_axis, z_axis, "g-");
        plt::xlabel("Samples");
        plt::ylabel("Acc");
        plt::legend();
        plt::show();
        return 0;
}
