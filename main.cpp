#include <iostream>
#include <iio.h>
#include <errno.h>
#include <string.h>
#include <vector>
#include "matplotlibcpp.h"

namespace plt = matplotlibcpp;

#define URI "ip:10.76.84.245"
#define DEV_NAME "ad5592r_s"
#define SAMPLE_COUNT 32
#define TRESHOLD 100
#define SAMPLE_FREQUENCY 50

typedef struct{
        uint16_t xpos;
        uint16_t xneg;

        uint16_t ypos;
        uint16_t yneg;

        uint16_t zpos;
        uint16_t zneg;
}current_samples;

int main (int argc, char** argv){

        struct iio_channel *channels[6];

        //URI
        struct iio_context* ctx = iio_create_context_from_uri(URI);
        if(!ctx){
                printf("Context no work bro, error: %s\n",strerror(errno));
                return -errno;
        }
        
        //MODIFY SAMPLE FREQUENCY
        struct iio_device *trigger = iio_context_find_device(ctx, "trigger0");
        if(!trigger){
                std::cerr<< "Didn't find trigger bro"<<std::endl;
                return -1;
        }

        double freq;
        int res = iio_device_attr_read_double(trigger, "sampling_frequency", &freq);
        if(freq != SAMPLE_FREQUENCY)
                iio_device_attr_write_double(trigger, "sampling_frequency", SAMPLE_FREQUENCY);

        //DEVICE
        struct iio_device* dev = iio_context_find_device(ctx, DEV_NAME);
        if(!dev){
                printf("Device no work bro");
                return -1;
        }

        //CHANNEL
        channels[0] = iio_device_find_channel(dev, "voltage0", false);
        if(!channels[0]){
                printf("Channel 0 no work bro");
                return -2;
        }

        channels[1] = iio_device_find_channel(dev, "voltage1", false);
        if(!channels[1]){
                printf("Channel 1 no work bro");
                return -2;
        }

        channels[2] = iio_device_find_channel(dev, "voltage2", false);
        if(!channels[2]){
                printf("Channel 2 no work bro");
                return -2;
        }

        channels[3] = iio_device_find_channel(dev, "voltage3", false);
        if(!channels[3]){
                printf("Channel 3 no work bro");
                return -2;
        }

        channels[4] = iio_device_find_channel(dev, "voltage4", false);
        if(!channels[4]){
                printf("Channel 4 no work bro");
                return -2;
        }

        channels[5] = iio_device_find_channel(dev, "voltage5", false);
        if(!channels[5]){
                printf("Channel 5 no work bro");
                return -2;
        }

        for (int i=0;i<6;i++){
                iio_channel_enable(channels[i]);
                std::cout<<"Am dat ineibal la canalul " << i<<std::endl;
        }

        //BUFFER
        iio_buffer *buff = iio_device_create_buffer(dev, SAMPLE_COUNT, false);
        if(!buff){
                std::cerr << "Could not create buffer, errno: "<< errno <<std::endl;
        }

        ssize_t byte_read = iio_buffer_refill(buff);
        if(byte_read < 0){
                std::cerr << "Could not refill buffer, errno: "<< byte_read <<std::endl;
        }

        //PARSING THROUGH THE BUFFER
        uint16_t step = iio_buffer_step(buff);
        std::vector<int> x_axis(SAMPLE_COUNT), y_axis(SAMPLE_COUNT), z_axis(SAMPLE_COUNT), t_axis(SAMPLE_COUNT);
        int count = 0;

        for(uint16_t *sample = static_cast<uint16_t *>(iio_buffer_start(buff)); sample < iio_buffer_end(buff); sample += step){
                current_samples samples;
                // samples.xpos = *sample;
                // samples.xneg = *(sample + 1);
                // samples.ypos = *(sample + 2);
                // samples.yneg = *(sample + 3);
                // samples.zpos = *(sample + 4);
                // samples.zneg = *(sample + 5);
                memcpy(&samples, sample, sizeof(samples));
                 
                //std::cout <<samples.xpos << " " <<samples.xneg << " " <<samples.ypos << " " <<samples.yneg << " " <<samples.zpos << " " <<samples.zneg << std::endl;
                
                x_axis.push_back(samples.xpos - samples.xneg);
                y_axis.push_back(samples.ypos - samples.yneg);
                z_axis.push_back(samples.zpos - samples.zneg);
                t_axis.push_back(count++);
        }

        // for(int i=1;i<x_axis.size();++i){
        //         int x_diff = abs(x_axis[i] - x_axis[i-1]);
        //         if(x_diff >= TRESHOLD)
        //                 std::cout<< "X axis exceeded treshold"<<std::endl;

        //         int y_diff = abs(y_axis[i] - y_axis[i-1]);
        //         if(y_diff >= TRESHOLD)
        //                 std::cout<< "Y axis exceeded treshold"<<std::endl;

        //         int z_diff = abs(z_axis[i] - z_axis[i-1]);
        //         if(z_diff >= TRESHOLD)
        //                 std::cout<< "Z axis exceeded treshold"<<std::endl;
        // }

        iio_buffer_destroy(buff);
        iio_context_destroy(ctx);

        plt::figure_size(1000, 600);
        plt::named_plot("X", t_axis, x_axis, "b-");
        plt::named_plot("Y", t_axis, y_axis, "r-");
        plt::named_plot("Z", t_axis, z_axis, "g-");
        plt::xlabel("Samples");
        plt::ylabel("Acc");
        plt::legend();
        plt::show();

        printf("Totu' bn\n");

        return 0;
}