#include <iostream>
#include <iio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <vector>
#include <memory>

#include "matplotlibcpp.h"

#define URI "ip:10.76.84.217"
#define DEV_NAME "ad5592r_s"
#define SAMPLE_COUNT 32
#define THRESHOLD 100
#define SAMPLE_FREQUENCY 50
namespace plt= matplotlibcpp;

typedef struct {
uint16_t xpos;
uint16_t xneg;
uint16_t ypos;  
uint16_t yneg;
uint16_t zpos;
uint16_t zneg;
} current_samples;

int main(int argc, char **argv){
struct iio_channel *channels[6];
const char *channels_name[]={"voltage0","voltage1","voltage2","voltage3","voltage4","voltage5"};
struct iio_context *ctx  =  iio_create_context_from_uri(URI);

if(!ctx){
std::cerr<<"ERROR: Context doesn't work, errno"<<errno<<std::endl;
return -errno;
}

//modify sample freq
struct iio_device *trigger=iio_context_find_device(ctx, "trigger0");
if(!trigger){
std::cerr<<"did not find trigger"<<errno<<std::endl;

return 1;
}



struct iio_device* dev = iio_context_find_device(ctx, DEV_NAME);
if(!dev){
printf("Device error\n");
return -1;
}

double val=0;
int write_trig ;
int read_trig = iio_device_attr_read_double(trigger,"sampling_frequency", &val);
if(read_trig<0){
        std::cerr<<"Read error"<<std::endl;
        return -1;
}
else {
        if (val != SAMPLE_FREQUENCY )
                write_trig = iio_device_attr_write_double(trigger,"sampling_frequency", SAMPLE_FREQUENCY);
                        
        if(write_trig<0){
        std::cerr<<"write error"<<std::endl;
        return -1;
}

}


for(int i=0; i<6; i++)
{      
channels[i] = iio_device_find_channel(dev, channels_name[i], false);
if(!channels[i]){
std::cout<<"Channel "<<i<< " error"<<std::endl;
return -1;
}
iio_channel_enable(channels[i]);
}
std::cout<<"Channels good"<<std::endl;

iio_buffer *buf =iio_device_create_buffer(dev, SAMPLE_COUNT,false);
if(!buf)
{
std::cerr<<"could not create buffer,errno: "<<errno<<std::endl;     
}

ssize_t byte_read =iio_buffer_refill(buf);
if(byte_read <0){
std :: cerr<<"could not refill,buffer,errno: "<<errno<< std::endl;
}

int count =0;
std::vector <int> x_axis,y_axis,z_axis,t_axis;

ptrdiff_t step_size = iio_buffer_step(buf);  
for(uint16_t *sample = static_cast<uint16_t *>(iio_buffer_start(buf));
        sample < iio_buffer_end(buf); sample +=step_size){
         
                
 current_samples current_sample; 

memcpy(&current_sample,sample,sizeof(current_samples));

// current_sample.xpos= *sample;
// current_sample.xneg= *(sample + 1);
// current_sample.ypos= *(sample + 2 );
// current_sample.yneg= *(sample + 3 );
// current_sample.zpos= *(sample + 4 );
// current_sample.zneg= *(sample + 5 );


// std::cout <<"+X:"<<current_sample.xpos << " " <<"-X:"<<current_sample.xneg<< " "
// <<"+Y:"<<current_sample.ypos << " " <<"-Y:"<<current_sample.yneg<< " "
// <<"+Z:"<<current_sample.zpos << " " <<"-Z:"<<current_sample.zneg<< "\n";

x_axis.push_back(current_sample.xpos-current_sample.xneg);
y_axis.push_back(current_sample.ypos-current_sample.yneg);
z_axis.push_back(current_sample.zpos-current_sample.zneg);
t_axis.push_back(count++);
}


// for(size_t i=1;i< x_axis.size();++i){
//         int x_diff = abs(x_axis[i]- x_axis[i - 1]);
//         if(x_diff>= THRESHOLD)
//         std:: cout <<"X axis exceeded threshold\n";

//         int y_diff = abs(y_axis[i]- y_axis[i - 1]);
//         if(y_diff>= THRESHOLD)
//         std:: cout <<"Y axis exceeded threshold\n";

//         int z_diff = abs(z_axis[i]- z_axis[i - 1]);
//         if(z_diff>= THRESHOLD)
//         std:: cout <<"Z axis exceeded threshold\n";
// }


plt::figure_size(1000,600);
plt::named_plot("X",t_axis,x_axis,"b-");
plt::named_plot("Y",t_axis,y_axis,"r-");
plt::named_plot("Z",t_axis,z_axis,"g-");
plt::xlabel("Samples");
plt::ylabel("Acc");
plt::legend();
plt::show();
iio_buffer_destroy(buf);
iio_context_destroy(ctx);
return 0;
}