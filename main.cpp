#include <iostream>
#include <iio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <vector>
#include "matplotlibcpp.h"

#define URI "ip:10.76.84.234"
#define DEV "ad5592r_s"
#define SAMP_COUNT 200
#define TRESHOLD 200
#define SAMPLE_FREQ 100

namespace plt = matplotlibcpp;

typedef struct
{

        uint16_t xpos;
        uint16_t xneg;
        uint16_t ypos;
        uint16_t yneg;
        uint16_t zpos;
        uint16_t zneg;

} current_samples;

int main(int argc, char **argv)
{


        struct iio_context *ctx = iio_create_context_from_uri(URI);

         if (!ctx)
         {
                std::cout<<"Ai bai la context ficior. Eroare: " <<strerror(errno);
                return -errno;
         }


         //Sample freq

         struct iio_device *trigger = iio_context_find_device(ctx, "trigger0");
        if (!ctx)
         {
                std::cout<<"Ai bai la trigger ficior.";
                return -1;
         }

        double read_freq;
        int check;
        check = iio_device_attr_read_double(trigger, "sampling_frequency", &read_freq);
        if(check < 0) {
                printf("Bai la sampling \n");
                return -1;
        }
        else {
                if (read_freq != SAMPLE_FREQ)
                        iio_device_attr_write_double(trigger, "sampling_frequency", SAMPLE_FREQ);
        }
         

         struct iio_device *dev = iio_context_find_device(ctx, DEV);
        if (!dev)
        {
                std::cout<<"Ai bai la device flacau \n";
                return -1;
        }


        struct iio_channel *channels[6];

        for(int i=0;i<6;i++)
        {
                std::string name = "voltage" + std::to_string(i);
                channels[i] = iio_device_find_channel(dev, name.c_str(), false);
              

                 if (!channels[i])
                {
                 std::cout<<"Ai bai la channel "<< i <<" ficior. Eroare: " << strerror(errno);
                return -errno;
                }

         iio_channel_enable(channels[i]);
        }


        iio_buffer *buff = iio_device_create_buffer(dev, SAMP_COUNT, false);

        if(!buff)
        {
                std::cerr << "Bai la buffer, errno: "<< errno<<'\n';
        }

        int byte_read = iio_buffer_refill(buff);
        if(!byte_read)
        {
                std::cerr <<"Bai la buffer refill, errno: "<<errno<<'\n';
        }

        //start, step, end
        uint16_t step_size = iio_buffer_step(buff);
        current_samples current_sample;

            std::vector<int> x_axis(SAMP_COUNT), y_axis(SAMP_COUNT),z_axis(SAMP_COUNT),t_axis(SAMP_COUNT);
           
            int count=0;

        for(uint16_t *sample = static_cast<uint16_t *>(iio_buffer_start(buff)); sample < iio_buffer_end(buff); sample += step_size)
        {
                
                memcpy(&current_sample, sample, 6 * sizeof(uint16_t));

                std::cout<<current_sample.xpos<< " " << current_sample.xneg<< " ";
                std::cout<<current_sample.ypos << " " << current_sample.yneg<< " ";
                std::cout<<current_sample.zpos << " " << current_sample.zneg<< '\n';

                x_axis.push_back(current_sample.xpos - current_sample.xneg);
                y_axis.push_back(current_sample.ypos - current_sample.yneg);
                z_axis.push_back(current_sample.zpos - current_sample.zneg);
                t_axis.push_back(count++);
        }

        iio_buffer_destroy(buff);
        iio_context_destroy(ctx);

        
        // for(size_t i=1;i<x_axis.size(); i++)
        // {
        //         int x_diff = abs(x_axis[i]-x_axis[i-1]);

        //         if(x_diff >= TRESHOLD)
        //                 std::cout<<"X axes exceeded treshold \n";


        //         int y_diff = abs(y_axis[i]-y_axis[i-1]);

        //         if(y_diff >= TRESHOLD)
        //                 std::cout<<"Y axes exceeded treshold \n";

                
        //         int z_diff = abs(z_axis[i]-z_axis[i-1]);

        //         if(z_diff >= TRESHOLD)
        //                 std::cout<<"Z axes exceeded treshold \n";
                
                


        // }

        plt::figure_size(1000,600);
        plt::named_plot("X", t_axis, x_axis,"b-");
        plt::named_plot("Y", t_axis, y_axis,"r-");      
        plt::named_plot("Z", t_axis, z_axis,"g-");

        plt::xlabel("Samples");
        plt::ylabel("Acc");
        plt::legend();
        plt::show();

        std::cout<<"\n bun \n";
        return 0;
}





//              ⠀⣠⣤⣤⣤⣤⣤⣤⣤⣤⣄⡀⠀⠀⠀⠀⠀⠀⠀⠀ SUS
// ⠀⠀⠀⠀⠀⠀⠀⠀⢀⣴⣿⡿⠛⠉⠙⠛⠛⠛⠛⠻⢿⣿⣷⣤⡀⠀⠀⠀⠀⠀ 
// ⠀⠀⠀⠀⠀⠀⠀⠀⣼⣿⠋⠀⠀⠀⠀⠀⠀⠀⢀⣀⣀⠈⢻⣿⣿⡄⠀⠀⠀⠀ 
// ⠀⠀⠀⠀⠀⠀⠀⣸⣿⡏⠀⠀⠀⣠⣶⣾⣿⣿⣿⠿⠿⠿⢿⣿⣿⣿⣄⠀⠀⠀ 
// ⠀⠀⠀⠀⠀⠀⠀⣿⣿⠁⠀⠀⢰⣿⣿⣯⠁⠀⠀⠀⠀⠀⠀⠀⠈⠙⢿⣷⡄⠀ 
// ⠀⠀⣀⣤⣴⣶⣶⣿⡟⠀⠀⠀⢸⣿⣿⣿⣆⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣿⣷⠀ 
// ⠀⢰⣿⡟⠋⠉⣹⣿⡇⠀⠀⠀⠘⣿⣿⣿⣿⣷⣦⣤⣤⣤⣶⣶⣶⣶⣿⣿⣿⠀ 
// ⠀⢸⣿⡇⠀⠀⣿⣿⡇⠀⠀⠀⠀⠹⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⠃⠀ 
// ⠀⣸⣿⡇⠀⠀⣿⣿⡇⠀⠀⠀⠀⠀⠉⠻⠿⣿⣿⣿⣿⡿⠿⠿⠛⢻⣿⡇⠀⠀ 
// ⠀⣿⣿⠁⠀⠀⣿⣿⡇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢸⣿⣧⠀⠀ 
// ⠀⣿⣿⠀⠀⠀⣿⣿⡇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢸⣿⣿⠀⠀ 
// ⠀⣿⣿⠀⠀⠀⣿⣿⡇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢸⣿⣿⠀⠀ 
// ⠀⢿⣿⡆⠀⠀⣿⣿⡇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢸⣿⡇⠀⠀ 
// ⠀⠸⣿⣧⡀⠀⣿⣿⡇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣿⣿⠃⠀⠀ 
// ⠀⠀⠛⢿⣿⣿⣿⣿⣇⠀⠀⠀⠀⣰⣿⣿⣷⣶⣶⣶⣶⠶⠀⢠⣿⣿⠀⠀⠀ 
// ⠀⠀⠀⠀⠀⠀⠀⣿⣿⠀⠀⠀⠀⠀⣿⣿⡇⠀⣽⣿⡏⠁⠀⠀⢸⣿⡇⠀⠀⠀ 
// ⠀⠀⠀⠀⠀⠀⠀⣿⣿⠀⠀⠀⠀⠀⣿⣿⡇⠀⢹⣿⡆⠀⠀⠀⣸⣿⠇⠀⠀⠀ 
// ⠀⠀⠀⠀⠀⠀⠀⢿⣿⣦⣄⣀⣠⣴⣿⣿⠁⠀⠈⠻⣿⣿⣿⣿⡿⠏⠀⠀⠀⠀ 
// ⠀⠀⠀⠀⠀⠀⠀⠈⠛⠻⠿⠿⠿⠿⠋