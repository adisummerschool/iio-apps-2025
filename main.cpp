#include<iostream>
#include<iio.h>
#include<string.h>
#include<vector>

#define URI "ip:10.76.84.118"
#define DEV_NAME "ad5592r_s"
#define SAMPLE_COUNT 32

#define THRESHOLD 100
#define SAMPLE_FREQUENCY 50

typedef struct {
        uint16_t x_pos;
        uint16_t x_neg;
        
        uint16_t y_pos;
        uint16_t y_neg;

        uint16_t z_pos;
        uint16_t z_neg;
} current_samples;

int main(int argc, char** argv){
        struct iio_channel * channels[6];
        struct iio_context * ctx = iio_create_context_from_uri(URI);

        struct iio_device * trigger = iio_context_find_device(ctx, "trigger0");
        double aux_trigger_freq;
        iio_device_attr_read_double(trigger, "sampling_frequency", &aux_trigger_freq);
        if(aux_trigger_freq != SAMPLE_FREQUENCY){
                iio_device_attr_write_double(trigger, "sampling_frequency", SAMPLE_FREQUENCY);
        }

        if(!trigger){
                perror("Trigger not found");
                return 0;
        }

        

        struct iio_device * dev = iio_context_find_device(ctx, DEV_NAME);

        if(!dev){
                perror("Device not found");
                return 0;
        }

        channels[0] = iio_device_find_channel(dev, "voltage0", false);
        if (!channels[0]){
                printf("Nu gasesc channel-ul rege\n");
                return 0;
        }

        channels[1] = iio_device_find_channel(dev, "voltage1", false);
        if (!channels[1]){
                printf("Nu gasesc channel-ul rege\n");
                return 0;
        }
        
        channels[2] = iio_device_find_channel(dev, "voltage2", false);
        if (!channels[2]){
                printf("Nu gasesc channel-ul rege\n");
                return 0;
        }

        channels[3] = iio_device_find_channel(dev, "voltage3", false);
        if (!channels[3]){
                printf("Nu gasesc channel-ul rege\n");
                return 0;
        }

        channels[4] = iio_device_find_channel(dev, "voltage4", false);
        if (!channels[4]){
                printf("Nu gasesc channel-ul rege\n");
                return 0;
        }

        channels[5] = iio_device_find_channel(dev, "voltage5", false);
        if (!channels[5]){
                printf("Nu gasesc channel-ul rege\n");
                return 0;       
        }

        for(int i = 0; i < 6; i++ ){
                iio_channel_enable(channels[i]);
                printf("Da ma da, canalul asta: %d, merge \n" , i);
        }

        iio_buffer *buf = iio_device_create_buffer(dev, SAMPLE_COUNT, false);
        if(!buf){
                std::cerr << "Couldn't create buffer , errno: " << errno << std::endl;
        } 

        ssize_t byte_read = iio_buffer_refill(buf);
        if(byte_read < 0){
                std::cerr << "Couldn't refill buffer , errno: " << errno << std::endl;
        }

        
        //trebe puse intr un vector
        std::vector<int> x_axis(SAMPLE_COUNT),
        y_axis(SAMPLE_COUNT),
        z_axis(SAMPLE_COUNT);

        // functii de folosit: iio_buffer_start sau iio_buffer_frst (cu specificare de unde incepe),
        // cu start le vedem pe toate pana la iio_buffer_end
        uint16_t step_size = iio_buffer_step(buf);
        for(uint16_t *sample = static_cast<uint16_t*>(iio_buffer_start(buf)); 
        sample < iio_buffer_end(buf); 
        sample += step_size)
        {
                current_samples current_sample;
                memcpy(&current_sample, sample, sizeof(current_sample));
                // current_samples -> x_pos = *sample;
                // current_samples -> x_neg = *(sample + sizeof(u_int16_t)); 
                // current_samples -> y_pos = *(sample + 2 * sizeof(u_int16_t));
                // current_samples -> y_neg = *(sample + 3 * sizeof(u_int16_t));
                // current_samples -> z_pos = *(sample + 4 * sizeof(u_int16_t));
                // current_samples -> z_neg = *(sample + 5 * sizeof(u_int16_t));
                
                std::cout << current_sample.x_pos << " " <<  current_sample.x_neg << " "
                          << current_sample.y_pos << " " << current_sample.y_neg << " "
                          << current_sample.z_pos << " " << current_sample.z_neg << "\n ";

                x_axis.push_back(current_sample.x_pos - current_sample.x_neg);
                y_axis.push_back(current_sample.y_pos - current_sample.y_neg);
                z_axis.push_back(current_sample.z_pos - current_sample.z_neg);


        }

        for(int i = 1; i < x_axis.size(); ++i){
                int x_diff = abs(x_axis[i] - x_axis[i-1]);
                if(x_diff >= THRESHOLD){
                        std::cout << "X axis excedeed threshold\n";
                }

                int y_diff = abs(y_axis[i] - y_axis[i-1]);
                if(y_diff >= THRESHOLD){
                        std::cout << "Y axis excedeed threshold\n";
                }

                int z_diff = abs(z_axis[i] - z_axis[i-1]);
                if(z_diff >= THRESHOLD){
                        std::cout << "Z axis excedeed threshold\n";
                }
                
        }

        return 0;
}