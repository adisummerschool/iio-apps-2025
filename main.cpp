#include <cstdint>
#include <iostream>
#include <iio.h>
#include <errno.h>
#include <string.h>
#include <string>
#include <vector>
#include "matplotlibcpp.h"

namespace plt = matplotlibcpp;

#define URI "ip:10.76.84.157"
#define DEV_NAME "ad5592r_s"
#define SAMPLE_COUNT 32
#define THRESHOLD 100
#define SAMPLE_FREQUENCY 50

typedef struct {
        uint16_t xpos;
        uint16_t xneg;

        uint16_t ypos;
        uint16_t yneg;

        uint16_t zpos;
        uint16_t zneg;
} current_samples;

int main(int argc, char**argv){
        iio_channel *channels[6];
        std::string str = "voltage";
        double readval;
        double sampling_freq;
        int errcode;
        int counter = 0;
        std::vector<int> time;
        std::vector<int> x_axis;
        std::vector<int> y_axis;
        std::vector<int> z_axis;


        iio_context* ctx = iio_create_context_from_uri(URI);

        if(!ctx)
        {
                printf("Eroarea Context: %s\n", strerror(errno));
                return -errno;
        }


        //Modify Sample Frequency

        struct iio_device* tr = iio_context_find_device(ctx, "trigger0");
        if(!tr)
        {
                printf("Trigger not found!\n");
                return 1;
        }

        int trig_err = iio_device_attr_read_double(tr, "sampling_frequency", &sampling_freq);

        if(trig_err!=0){
                printf("Sampling frequency couldn't be read %d\n", trig_err);
        }else if(sampling_freq != SAMPLE_FREQUENCY)
        {
                trig_err = iio_device_attr_write_double(tr, "sampling_frequency", SAMPLE_FREQUENCY);
                if(trig_err!=0)
                {
                        printf("Couldn't write to trigger %d\n", trig_err);
                }
        }



        struct iio_device* dev = iio_context_find_device(ctx, DEV_NAME);
        if(!dev){
                        printf("Eroare device \n");
                        return -1;
        }


        for(int i=0; i<6; i++)
        {
                str = "voltage";
                str += std::to_string(i);
                channels[i] = iio_device_find_channel(dev, str.c_str(), false);
                if(!channels[i])
                {
                        printf("Eroare canalul %d\n", i);
                        return -1;
                }
                iio_channel_enable(channels[i]);

        }

        iio_buffer* buf = iio_device_create_buffer(dev, SAMPLE_COUNT, false);
        if(!buf)
        {
                printf("Could not create buffer, errno: %d\n", errno);
        }

        ssize_t bytes_read = iio_buffer_refill(buf);
        if(!bytes_read)
        {
                printf("Buffer refill error %d\n", bytes_read);
                return bytes_read;
        }

        void* start_point = iio_buffer_start(buf);
        void* end_point = iio_buffer_end(buf);
        ptrdiff_t step = iio_buffer_step(buf)/sizeof(uint16_t);
        printf("xpos\txneg\typos\tyneg\tzpos\tzneg\n\n");
        for(uint16_t* i = (uint16_t *)start_point ; i<(uint16_t*)end_point; i+=step)
        {
                current_samples current_sample;
                memcpy(&current_sample, i, sizeof(current_samples));
                //current_sample.xpos = *i;
                //current_sample.xneg = *(i + 1);
                //x_axis[counter] = current_sample.xpos - current_sample.xneg;
                x_axis.push_back(current_sample.xpos - current_sample.xneg);
                //current_sample.ypos = *(i + 2);
                //current_sample.yneg = *(i + 3);
                //y_axis[counter] = current_sample.ypos - current_sample.yneg;
                y_axis.push_back(current_sample.ypos - current_sample.yneg);

                //current_sample.zpos = *(i + 4);
                //current_sample.zneg = *(i + 5);
                //z_axis[counter] = current_sample.zpos - current_sample.zneg;
                z_axis.push_back(current_sample.zpos - current_sample.zneg);

                time.push_back(counter++);


                printf("%d\t%d\t%d\t%d\t%d\t%d\n",current_sample.xpos,
                current_sample.xneg, current_sample.ypos, current_sample.yneg, current_sample.zpos,
                 current_sample.zneg);
        }

        // for (int i=1; i < x_axis.size(); ++i)
        // {
                
        //         int x_diff = x_axis[i] - x_axis[i-1];
        //         if (x_diff >= THRESHOLD)
        //         {
        //                 printf("%d\t", i);
        //                 printf("X axis exceeded threshold\n");
        //         }
        //         int y_diff = y_axis[i] - y_axis[i-1];
        //         if (y_diff >= THRESHOLD)
        //         {
        //                 printf("%d\t", i);
        //                 printf("Y axis exceeded threshold\n");
        //         }
        //         int z_diff = z_axis[i] - z_axis[i-1];
        //         if (z_diff >= THRESHOLD)
        //         {
        //                 printf("%d\t", i);
        //                 printf("Z axis exceeded threshold\n");
        //         }
        // }
        //printf("x_axis size: %d\n", x_axis.size());

        plt::figure_size(1000, 600);
        plt::named_plot("X", time, x_axis, "b-");
        plt::named_plot("Y", time, y_axis, "r-");
        plt::named_plot("Z", time, z_axis, "g-");
        plt::xlabel("Sample");
        plt::ylabel("Acc");
        plt::legend();
        plt::show();
        return 0;
}