#include <iostream>
#include <iio.h>
#include <cstring>
#include <vector>

#define URI "ip:10.76.84.229"
#define DEV_NAME "ad5592r-s"
#define NUM_CHANNELS 6

#define THRESHOLD 100
#define SAMPLE_COUNT 32
#define SAMPLING_FREQUENCY 50

typedef struct {
	uint16_t xpos;
	uint16_t xneg;

	uint16_t ypos;
	uint16_t yneg;

	uint16_t zpos;
	uint16_t zneg;
} current_samples;

int main(int argc, char **argv) {
	// 1. URI + DEV
	struct iio_context *ctx = iio_create_context_from_uri(URI);
	if (!ctx) {
		std::cerr << "Context create failed, errno %d" << errno << '\n';
		return -1;
	}

	// Modify sampling freqency
	struct iio_device *trigger = iio_context_find_device(ctx, "trigger0");
	if (!trigger) {
		std::cerr << "Context find trigger failed\n";
		return -1;
	}
	
	double val = -1;
	int read_freq = iio_device_attr_read_double(trigger, "sampling_frequency", &val);
	if (read_freq) {
		std::cerr << "Read sampling freqency failed";
		return -1;
	}

	if (val != SAMPLING_FREQUENCY) {
		int write_freq = iio_device_attr_write_double(trigger, "sampling_frequency", SAMPLING_FREQUENCY);
		if (write_freq) {
			std::cerr << "Write sampling freqency failed";
			return -1;
		}
	}

	struct iio_device *dev = iio_context_find_device(ctx, DEV_NAME);
	if (!dev) {
		std::cerr << "Context find device failed\n";
		return -1;
	}

	// 2. Enable channels
	struct iio_channel *channel[NUM_CHANNELS];
	std::string voltages[NUM_CHANNELS] = {"voltage0", "voltage1", "voltage2",
		"voltage3", "voltage4", "voltage5"};

	for (int i = 0; i < NUM_CHANNELS; ++i) {
		channel[i] = iio_device_find_channel(dev, voltages[i].c_str(), false);
		if (!channel[i]) {
			std::cerr << "Find device failed\n";
			return -1;
		}
		iio_channel_enable(channel[i]);
	}

	// 3. Create buffer (cyclic ? reads continously : reads once)
	struct iio_buffer *buffer = iio_device_create_buffer(dev, SAMPLE_COUNT, false);
	if (!buffer) {
		std::cerr << "Create buffer failed, errno %d" << errno << '\n';
		return -1;
	}

	// 4. Refill
	ssize_t byte_read = iio_buffer_refill(buffer);
	if (byte_read < 0) {
		std::cerr << "Buffer refill failed, errno %d" << errno << '\n';
		return -1;
	}

	// 5. Add to vector
	ptrdiff_t step_size = iio_buffer_step(buffer);
	current_samples current_sample;
	std::vector<int> x_axis, y_axis, z_axis;

	for (uint16_t *sample = static_cast<uint16_t*>(iio_buffer_start(buffer)); 
		sample < iio_buffer_end(buffer); sample += step_size) {
			std::memcpy(&current_sample, sample, sizeof(current_samples));

			// current_sample.xpos = *sample;
			// current_sample.xneg = *(sample + sizeof(uint16_t));

			// current_sample.ypos = *(sample + 2 * sizeof(uint16_t));
			// current_sample.yneg = *(sample + 3 * sizeof(uint16_t));

			// current_sample.zpos = *(sample + 4 * sizeof(uint16_t));
			// current_sample.zneg = *(sample + 5 * sizeof(uint16_t));

			std::cout << current_sample.xpos << " " << current_sample.xneg << " "
				<< current_sample.ypos << " " << current_sample.yneg << " "
				<< current_sample.zpos << " " << current_sample.zneg << "\n";
			
			x_axis.push_back(current_sample.xpos - current_sample.xneg);
			y_axis.push_back(current_sample.ypos - current_sample.yneg);
			z_axis.push_back(current_sample.zpos - current_sample.zneg);
	}

	uint16_t x_diff = 0;
	uint16_t y_diff = 0;
	uint16_t z_diff = 0;
	for (size_t i = 1; i < x_axis.size(); ++i) {
		x_diff = abs(x_axis[i] - x_axis[i - 1]);
		y_diff = abs(y_axis[i] - y_axis[i - 1]);
		z_diff = abs(z_axis[i] - z_axis[i - 1]);

		if (x_diff >= THRESHOLD) {
			std::cout << "X axis exceeded threshold\n";
		}
		if (y_diff >= THRESHOLD) {
			std::cout << "Y axis exceeded threshold\n";
		}
		if (z_diff >= THRESHOLD) {
			std::cout << "Z axis exceeded threshold\n";
		}
	}

	iio_context_destroy(ctx);
	return 0;
}