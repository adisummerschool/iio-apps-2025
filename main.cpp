#include <iostream>
#include "matplotlibcpp.h"
#include <iio.h>
#include <errno.h>
#include <string.h>
#include <vector>

#define URI "ip:10.76.84.213"
#define DEV_NAME "ad5592r_s"
#define SAMPLE_COUNT 32
#define THRESHOLD 100

namespace plt = matplotlibcpp;

typedef struct {
	uint16_t xpos;
	uint16_t xneg;

	uint16_t ypos;
	uint16_t yneg;

	uint16_t zpos;
	uint16_t zneg;
} current_samples;

int main (int argc, char **argv)
{
	struct iio_channel *channels[6];

	struct iio_context* ctx = iio_create_context_from_uri(URI);
	if (!ctx) {
        	printf("Context error %s\n", strerror(errno));
        	return -errno;
    	}

	struct iio_device *trig = iio_context_find_device(ctx, "trigger0");
	if (!trig) {
                printf("Trigger error\n");
                return -1;
        }

	ssize_t mbytes = iio_device_attr_write(trig, "sampling_frequency", "50");
	if(mbytes < 0){
		perror("Trigger attribute write failed");
    		return -1;
	}
	
	struct iio_device* dev = iio_context_find_device(ctx, DEV_NAME);
	if (!dev) {
        	printf("Device error\n");
        	return -1;
    	}


	char voltages[6][9] = {"voltage0", "voltage1", "voltage2",
			    "voltage3", "voltage4", "voltage5"};

	for(int i = 0; i < 6; i++){
		channels[i] = iio_device_find_channel(dev, voltages[i], false);
    		if (!channels[i]) {
        		printf("Channel error\n");
        		return -1;
    		}
		iio_channel_enable(channels[i]);
	}

	iio_buffer *buf = iio_device_create_buffer(dev, SAMPLE_COUNT, false);
	if (!buf) {
    		perror("Could not create buffer");
    		return -1;
	}

	int ret = iio_buffer_refill(buf);
	if (ret < 0) {
		std::cerr << "Buffer refill failed: " << strerror(-ret) << std::endl;
	}


	std::vector<uint16_t> x_axis;
	std::vector<uint16_t> y_axis;
	std::vector<uint16_t> z_axis;
	std::vector<int> t_axis;

	int count = 0;
	for(uint16_t *sample = static_cast<uint16_t *>(iio_buffer_start(buf));
		 sample < iio_buffer_end(buf); sample += iio_buffer_step(buf))
	{
		current_samples curr_s;

		curr_s.xpos = *sample;
		curr_s.xneg = *(sample + 1);

		curr_s.ypos = *(sample + 2);
		curr_s.yneg = *(sample + 3);

		curr_s.zpos = *(sample + 4);
		curr_s.zneg = *(sample + 5);

		std::cout << "X+: " << curr_s.xpos << "  X-: " << curr_s.xneg << "\n";
		std::cout << "Y+: " << curr_s.ypos << "  Y-: " << curr_s.yneg << "\n";
		std::cout << "Z+: " << curr_s.zpos << "  Z-: " << curr_s.zneg << "\n" << "\n" << "\n";

		x_axis.push_back(curr_s.xpos - curr_s.xneg);
		y_axis.push_back(curr_s.ypos - curr_s.yneg);
		z_axis.push_back(curr_s.zpos - curr_s.zneg);
		t_axis.push_back(count++);
	}

	for(size_t i = 1; i < x_axis.size(); ++i)
	{
		int x_diff = abs(x_axis[i] - x_axis[i-1]);
		if(x_diff >= THRESHOLD)
		{
			std::cout << "X axis exceeded threshold" << "\n";
		}
	}

	for(size_t i = 1; i < y_axis.size(); ++i)
        {
                int y_diff = abs(y_axis[i] - y_axis[i-1]);
                if(y_diff >= THRESHOLD)
                {
                        std::cout << "Y axis exceeded threshold" << "\n";
                }
        }

	for(size_t i = 1; i < z_axis.size(); ++i)
        {
                int z_diff = abs(z_axis[i] - z_axis[i-1]);
                if(z_diff >= THRESHOLD)
                {
                        std::cout << "Z axis exceeded threshold" << "\n";
                }
        }

	plt::figure_size(1000, 600);
	plt::named_plot("X", t_axis, x_axis, "b-");
	plt::named_plot("Y", t_axis, y_axis, "r-");
	plt::named_plot("Z", t_axis, z_axis, "g-");
	plt::xlabel("Samples");
	plt::ylabel("Acc");
	plt::legend();
	plt::show();

	iio_context_destroy(ctx);
	return 0;
}

