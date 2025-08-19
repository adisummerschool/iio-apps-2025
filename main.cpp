#include "matplotlibcpp.h"
#include <iostream>
#include <iio.h>
#include <errno.h>
#include <cstring>
#include <memory>
#include <vector>

#define URI "ip:10.76.84.127"
#define DEV_NAME "iio_ad5592r_s"
#define SAMPLE_COUNT 200
#define THRESHOLD 100
#define SAMPLE_FREQUENCY 100

namespace plt = matplotlibcpp;

typedef struct main {
	uint16_t xpos;
	uint16_t xneg;
	uint16_t ypos;
	uint16_t yneg;
	uint16_t zpos;
	uint16_t zneg;
} current_samples;
int main(int argc, char **agrv)
{
	struct iio_channel *channels[6];
	iio_create_context_from_uri(URI);
	struct iio_context *ctx = iio_create_context_from_uri(URI);
	if (!ctx) {
		printf("Vezi ba ca nu merge contextul , eroare %s\n",
		       strerror(errno));
		return -errno;
	}

	//Modify sample freq
	struct iio_device *trigger = iio_context_find_device(ctx, "trigger0");
	if (!trigger) {
		std::cerr << "Did not find trigger\n";
		return 1;
	}

	//int var = iio_device_attr_write(trigger, "sampling_ frequency", std::to_string(SAMPLE_FREQUENCY).c_str());
	// double sample_freq = 50.0;
	std::string freq_str = std::to_string(SAMPLE_FREQUENCY);
	iio_device_attr_write(trigger, "sampling_frequency", freq_str.c_str());

	struct iio_device *dev = iio_context_find_device(ctx, DEV_NAME);
	if (!dev) {
		printf("Cam naspa nuemele asta de device\n");
		return -1;
	}
	for (int i = 0; i < 6; i++) {
		switch (i) {
		case 0:
			channels[i] =
				iio_device_find_channel(dev, "voltage0", false);
			iio_channel_enable(channels[i]);
			if (!channels[i]) {
				printf("Ai gresit ceva idk\n");
				return -1;
			} else {
				double val;
				int res = iio_channel_attr_read_double(
					channels[i], "raw", &val);
				printf("Valoarea y-: %f \n", val);
			}
			break;
		case 1:
			channels[i] =
				iio_device_find_channel(dev, "voltage1", false);
			iio_channel_enable(channels[i]);
			if (!channels[i]) {
				printf("Ai gresit ceva idk\n");
				return -1;
			} else {
				double val;
				int res = iio_channel_attr_read_double(
					channels[i], "raw", &val);
				printf("Valoarea y+: %f \n", val);
			}
			break;
		case 2:
			channels[i] =
				iio_device_find_channel(dev, "voltage2", false);
			iio_channel_enable(channels[i]);
			if (!channels[i]) {
				printf("Ai gresit ceva idk\n");
				return -1;
			} else {
				double val;
				int res = iio_channel_attr_read_double(
					channels[i], "raw", &val);
				printf("Valoarea x+: %f \n", val);
			}
			break;
		case 3:
			channels[i] =
				iio_device_find_channel(dev, "voltage3", false);
			iio_channel_enable(channels[i]);
			if (!channels[i]) {
				printf("Ai gresit ceva idk\n");
				return -1;
			} else {
				double val;
				int res = iio_channel_attr_read_double(
					channels[i], "raw", &val);
				printf("Valoarea x-: %f \n", val);
			}
			break;
		case 4:
			channels[i] =
				iio_device_find_channel(dev, "voltage4", false);
			iio_channel_enable(channels[i]);
			if (!channels[i]) {
				printf("Ai gresit ceva idk\n");
				return -1;
			} else {
				double val;
				int res = iio_channel_attr_read_double(
					channels[i], "raw", &val);
				printf("Valoarea z+: %f \n", val);
			}
			break;
		case 5:
			channels[i] =
				iio_device_find_channel(dev, "voltage5", false);
			iio_channel_enable(channels[i]);
			if (!channels[i]) {
				printf("Ai gresit ceva idk\n");
				return -1;
			} else {
				double val;
				int res = iio_channel_attr_read_double(
					channels[i], "raw", &val);
				printf("Valoarea z-: %f \n", val);
			}
			break;
		}
	}
	std::vector<int> x_axis, y_axis, z_axis, t_axis;
	int count = 0;
	iio_buffer *buf = iio_device_create_buffer(dev, SAMPLE_COUNT, false);
	if (!buf) {
		std::cerr << "Could not create buffer, errno" << errno
			  << std::endl;
	}
	ssize_t bytes_read = iio_buffer_refill(buf);
	if (bytes_read < 0) {
		std ::cerr << "Could not refli, errno \n" << errno << std::endl;
	}
	ptrdiff_t step_size = iio_buffer_step(buf);
	for (uint16_t *sample = static_cast<uint16_t *>(iio_buffer_start(buf));
	     sample < iio_buffer_end(buf); sample += step_size) {
		current_samples current_sample;
		memcpy(&current_sample, sample, sizeof(current_sample));
		// current_sample.xpos = *sample;
		// current_sample.xneg = *(sample + 1);
		// current_sample.ypos = *(sample + 2);
		// current_sample.yneg = *(sample + 3);
		// current_sample.zpos = *(sample + 4);
		// current_sample.zneg = *(sample + 5);
		std::cout << current_sample.xpos << " " << current_sample.xneg
			  << " " << current_sample.ypos << " "
			  << current_sample.yneg << " " << current_sample.zpos
			  << " " << current_sample.zneg << "\n";

		x_axis.push_back(current_sample.xpos - current_sample.xneg);
		y_axis.push_back(current_sample.ypos - current_sample.yneg);
		z_axis.push_back(current_sample.zpos - current_sample.zneg);
		t_axis.push_back(count++);
	}

	// for (int i=0; i<x_axis.size(); ++i) {
	// int x_diff = x_axis[i] - x_axis[i -1];
	// if(x_diff >= THRESHOLD){
	// 	std::cout << "X axis excedded threshold\n";
	// }
	// }
	// for (int i=0; i<y_axis.size(); ++i) {
	// int y_diff = y_axis[i] - y_axis[i -1];
	// if(y_diff >= THRESHOLD){
	// 	std::cout << "Y axis excedded threshold\n";
	// }
	// }
	// for (int i=0; i<z_axis.size(); ++i) {
	// int z_diff = z_axis[i] - z_axis[i -1];
	// if(z_diff >= THRESHOLD){
	// 	std::cout << "Z axis excedded threshold\n";
	// }
	plt::figure_size(1000, 600);
	plt::named_plot("X", t_axis, x_axis, "b-");
	plt::named_plot("X", t_axis, y_axis, "r-");
	plt::named_plot("X", t_axis, z_axis, "g-");
	plt::xlabel("Samples");
	plt::ylabel("Acc");
	plt::legend();
	plt::show();

		//printf("Totul ok \n");
		return 0;
}
