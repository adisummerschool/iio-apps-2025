#include <iostream>
#include <iio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <cstring>
#include <vector>
#include <math.h>



#define URI "ip:10.76.84.218"
#define DEV_NAME "ad5592r_s"
#define SAMPLE_COUNT 32
#define THRESHOLD 100
#define SAMPLE_TRIG_FREQ 50

typedef struct {
        uint16_t xpos;
        uint16_t xneg;

        uint16_t ypos;
        uint16_t yneg;

        uint16_t zpos;
        uint16_t zneg;

}current_samples;

int main(int argc, char **argv){
        struct iio_channel *chan[6];
        current_samples curren_samp;
        std::vector<int> xvec, yvec,zvec;
        int counter=0;
        
        struct iio_context* ctx = iio_create_context_from_uri(URI);
        if (!ctx)
        {
                printf("err: %f\n", strerror(errno));
                return -errno;
        }
        struct iio_device *tr=iio_context_find_device(ctx,"trigger0");
        if(!tr){
                std::cerr<<"Did not find dev\n";
                return 1;
        }
        double sample_feq=0;

        int trigg_err=iio_device_attr_read_double(tr,"sampling_frequency",&sample_feq);
        if(trigg_err!=0)
        {
                printf("Sample freq err ");

        }else if(trigg_err==SAMPLE_TRIG_FREQ)
        {
                trigg_err=iio_device_attr_write_double(tr,"sampling_frequency",SAMPLE_TRIG_FREQ);
                if(trigg_err!=0)
                {
                        printf("Sample freq err x2 ");
                }
        }

        struct iio_device* dev=iio_context_find_device(ctx, DEV_NAME);
        if(!dev)
        {
                printf("naspa1\n");
                return -1;
        }
        
        
        chan[0]= iio_device_find_channel(dev, "voltage0", false);
        if(!chan[0])
        {
                printf("naspa2\n");
                return -1;
        }
        chan[1]= iio_device_find_channel(dev, "voltage1", false);
        if(!chan[1])
        {
                printf("naspa3\n");
                return -1;
        }
        chan[2]= iio_device_find_channel(dev, "voltage2", false);
        if(!chan[2])
        {
                printf("naspa4\n");
                return -1;
        }
        chan[3]= iio_device_find_channel(dev, "voltage3", false);
        if(!chan[3])
        {
                printf("naspa5\n");
                return -1;
        }
        chan[4]= iio_device_find_channel(dev, "voltage4", false);
        if(!chan[4])
        {
                printf("naspa6\n");
                return -1;
        }
        chan[5]= iio_device_find_channel(dev, "voltage5", false);
        if(!chan[5])
        {
                printf("naspa7\n");
                return -1;
        }
        for(int i=0;i<6;i++)
        {
                iio_channel_enable(chan[i]);
        }
        double val1,val2,val3,val4,val5,val0;
        
        iio_buffer *buf=iio_device_create_buffer(dev,SAMPLE_COUNT,false);
        if(!buf)
        {
                std::cerr<<"Could not create buf:"<<errno<<std::endl;
        }
        int buf_err=iio_buffer_refill(buf);
        if(buf_err<0)
        {
                std::cerr<<"Could not ref buf: "<<errno<<std::endl;
                return -100;
        }
        void* start=iio_buffer_start(buf);
        void* endd=iio_buffer_end(buf);
        ptrdiff_t step= iio_buffer_step(buf)/sizeof(uint16_t);
        for(uint16_t *i=(uint16_t*) start; i < (uint16_t*) endd; i += step)
        {
               std::memcpy(&curren_samp, i, sizeof(current_samples));
               //curren_samp.xpos=*i;
               //curren_samp.xneg=*(i+1);
        //        xvec[counter]=curren_samp.xpos-curren_samp.xneg;
        // //        curren_samp.ypos=*(i+2);
        // //        curren_samp.yneg=*(i+3);
        //        yvec[counter]=curren_samp.ypos-curren_samp.yneg;
        // //        curren_samp.zpos=*(i+4);
        // //        curren_samp.zneg=*(i+5);
        //        zvec[counter]=curren_samp.zpos-curren_samp.zneg;

        //        time[counter]=counter;
               std::cout<<"x: "<<curren_samp.xpos<<" "<<curren_samp.xneg<<"\n"
                        <<"y: "<<curren_samp.ypos<<" "<<curren_samp.yneg<<"\n"
                        <<"z: "<<curren_samp.zpos<<" "<<curren_samp.zneg<<"\n";

                xvec.push_back(curren_samp.xpos-curren_samp.xneg);
                yvec.push_back(curren_samp.ypos-curren_samp.yneg);
                zvec.push_back(curren_samp.zpos-curren_samp.zneg);

        }

        for(size_t i=1; i<xvec.size(); ++i)
        {
                int x_diff = abs(xvec[i] - xvec[i-1]);
                if(x_diff>=THRESHOLD){
                        std::cout<<"X axis exceeded th\n";
                }
                int y_diff = abs(yvec[i] - yvec[i-1]);
                if(y_diff>=THRESHOLD){
                        std::cout<<"y axis exceeded th\n";
                }
                int z_diff = abs(zvec[i] - zvec[i-1]);
                if(z_diff>=THRESHOLD){
                        std::cout<<"z axis exceeded th\n";
                }
        }
        
        return 0;
}