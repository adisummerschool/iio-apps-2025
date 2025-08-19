#include <iostream>
#include <iio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <vector>

#define URI "ip:10.76.84.231"
#define DEV_NAME "IIO_ADC_AD5592R_S"
#define SAMPLE_COUNT 32
#define THR 100
#define SAMPLE_FREQ 50

typedef struct {
    uint16_t xpoz;
    uint16_t xneg;
    uint16_t ypoz;
    uint16_t yneg;
    uint16_t zpoz;
    uint16_t zneg;
} current_samples;

int main(int argc, char *argv[]) {
    struct iio_channel *channels[6];
    
    const char* channel_names[6] = {
        "voltage0",    // xpoz
        "voltage1",    // xneg
        "voltage2",    // ypoz
        "voltage3",    // yneg
        "voltage4",    // zpoz
        "voltage5"     // zneg
    };
    
    struct iio_context* ctx;
    struct iio_device* dev;
    struct iio_buffer* buffer;

    double freq;

    int ret;
    double ret1;
    double values[6];
    
    ctx = iio_create_context_from_uri(URI);
    if (!ctx) {
        printf("Fratioare nu merge contextul fratioare, eroare: %s\n", strerror(errno));
        return -errno;
    }


//addjust SF

    struct iio_device *trigger=iio_context_find_device(ctx,"trigger0");
    if(!trigger){
        printf("Trigger fail\n");
        return -1;
    }

    ret1=iio_device_attr_read_double(trigger,"sampling_frequency",&freq);
    if(freq!=SAMPLE_FREQ){
        freq = iio_device_attr_write_double(trigger, "sampling_frequency", SAMPLE_FREQ);    
}
//
    
    
    dev = iio_context_find_device(ctx, DEV_NAME);
    if (!dev) {
        printf("Mie mi-ar fi rusine cu numele ala de device\n");
        iio_context_destroy(ctx);
        return -1;
    }

    for (int i = 0; i < 6; i++) {
        channels[i] = iio_device_find_channel(dev, channel_names[i], false);
        if (!channels[i]) {
            printf("Nu gasesc canalul %s, boss!\n", channel_names[i]);
            iio_context_destroy(ctx);
            return -1;
        }
        
        iio_channel_enable(channels[i]);

    }

    buffer = iio_device_create_buffer(dev, SAMPLE_COUNT, false);
    if (!buffer) {
        printf("Eroare creare buffer\n");
        iio_context_destroy(ctx);
        return -1;
    }
    
    ssize_t byte_read = iio_buffer_refill(buffer);
    if (byte_read < 0) {
        printf("Nu am citit valori\n");
        iio_buffer_destroy(buffer);
        iio_context_destroy(ctx);
        return -1;
    }


    ptrdiff_t step_size = iio_buffer_step(buffer);
    step_size /= sizeof(uint16_t);  
    std::vector<int> x_axis,y_axis,z_axis;

    for (uint16_t *sample = static_cast<uint16_t *>(iio_buffer_start(buffer));
         sample < iio_buffer_end(buffer); 
         sample += step_size) {
        
        current_samples current_sample;
        

        current_sample.xpoz = *sample;
        current_sample.xneg = *(sample + 1);    
        current_sample.ypoz = *(sample + 2);    
        current_sample.yneg = *(sample + 3);    
        current_sample.zpoz = *(sample + 4);    
        current_sample.zneg = *(sample + 5);    

        std::cout << "xpoz: " << current_sample.xpoz << " "
                  << "xneg: " << current_sample.xneg << " "
                  << "ypoz: " << current_sample.ypoz << " "
                  << "yneg: " << current_sample.yneg << " "
                  << "zpoz: " << current_sample.zpoz << " "
                  << "zneg: " << current_sample.zneg << "\n";

        x_axis.push_back(current_sample.xpoz - current_sample.xneg);
        y_axis.push_back(current_sample.ypoz - current_sample.yneg);
        z_axis.push_back(current_sample.zpoz - current_sample.zneg);


    }

    for(size_t i=1;i<x_axis.size();++i){
        int x_diff=abs(x_axis[i]-x_axis[i-1]);
                if(x_diff>=THR){
                        std::cout<<"X axis exceeded thhreshold\n";
                }
        int y_diff=abs(y_axis[i]-y_axis[i-1]);
                if(y_diff>=THR){
                        std::cout<<"Y axis exceeded thhreshold\n";
                }
        int z_diff=abs(z_axis[i]-z_axis[i-1]);
                if(z_diff>=THR){
                        std::cout<<"Z axis exceeded thhreshold\n";
                }
        printf("%d: \t%d\t%d\t%d\n", i, x_diff, y_diff, z_diff);
        
        }
    

    iio_buffer_destroy(buffer);
    iio_context_destroy(ctx);
    return 0;
}