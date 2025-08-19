#include <iostream>
#include <iio.h>
#include <string.h>
#include <memory>
#include <vector>

#define URI "ip:10.76.84.236"
#define DEV_NAME "ad5592r-s"
#define SAMPLE_COUNT 32
#define CHANNELS_NUM 6
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

int main(int argc, char **argv)
{
	//1. URI + DEVICE
	struct iio_context *ctx = iio_create_context_from_uri(URI);
	if (!ctx) {
		std::cerr << "Did not create context!\n";
		return -1;
	}

	// Modify sample frequency
	struct iio_device *trigger = iio_context_find_device(ctx, "trigger0");
	if (!trigger) {
		std::cerr << "Context find trigger failed!\n";
		return -1;
	}

	double val = -1;
	int read_freq = iio_device_attr_read_double(trigger, "sampling_frequency", &val);
	if (read_freq) {
		std::cerr << "Read sampling frequency failed\n";
		return -1;
	}

	if (val != SAMPLE_FREQUENCY) {
		int write_freq = iio_device_attr_write_double(trigger, "sampling_frequency", SAMPLE_FREQUENCY);
		if (write_freq) {
			std::cerr << "Write sampling frequency failed\n";
			return -1;
		}
	}

	struct iio_device *dev = iio_context_find_device(ctx, DEV_NAME);
	if (!dev) {
		std::cerr << "Did not create the device!\n";
		return -1;
	}

	//2. Enable channels
	struct iio_channel *channels[CHANNELS_NUM];
	std::string voltages[CHANNELS_NUM] = {"voltage0", "voltage1", "voltage2",
					      "voltage3", "voltage4", "voltage5"};
	
	for (int i = 0; i < CHANNELS_NUM; i++) {

		channels[i] = iio_device_find_channel(dev, voltages[i].c_str(), false);
		if (!channels[i]) {
			std::cout << "Channel error!";
			return -1;
		}
		iio_channel_enable(channels[i]);
	}

	//3. Buffer create; for the memory allocation
	struct iio_buffer *buffer = iio_device_create_buffer(dev, SAMPLE_COUNT, false);
	if (!buffer) {
		std::cerr << "Could not create buffer, errno: " << errno << std::endl;
		return -1;
	}

	ssize_t bytes_read = iio_buffer_refill(buffer);
	if (bytes_read < 0) {
		std::cerr << "Could not refill buffer, errno: " << errno << std::endl;
		return -1;
	}

	ptrdiff_t step_size = iio_buffer_step(buffer);
	current_samples *current_sample;
	std::vector<int> x_axis, y_axis, z_axis;

	for (uint16_t *sample = static_cast<uint16_t *>(iio_buffer_start(buffer)); sample < iio_buffer_end(buffer);
	     sample += step_size) {

		current_sample = (current_samples *) sample;

		// current_sample.x_pos = *sample;
		// current_sample.x_neg = *(sample + sizeof(uint16_t));

		// current_sample.y_pos = *(sample + 2 * sizeof(uint16_t));
		// current_sample.y_neg = *(sample + 3 * sizeof(uint16_t));

		// current_sample.z_pos = *(sample + 4 * sizeof(uint16_t));
		// current_sample.z_neg = *(sample + 5 * sizeof(uint16_t));

		std::cout << current_sample->x_pos << " " << current_sample->x_neg << " "
			  << current_sample->y_pos << " " << current_sample->y_neg << " "
			  << current_sample->z_pos << " " << current_sample->z_neg << "\n";

		x_axis.push_back(current_sample->x_pos - current_sample->x_neg);
		y_axis.push_back(current_sample->y_pos - current_sample->y_neg);
		z_axis.push_back(current_sample->z_pos - current_sample->z_neg);
	}

	for (size_t i = 0; i < x_axis.size(); i++) {

		uint16_t x_diff = abs(x_axis[i] - x_axis[i - 1]);
		if (x_diff >= THRESHOLD)
			std::cout << "X axis exceeded threshold\n";

		uint16_t y_diff = abs(y_axis[i] - y_axis[i - 1]);
		if (y_diff >= THRESHOLD)
			std::cout << "Y axis exceeded threshold\n";

		uint16_t z_diff = abs(z_axis[i] - z_axis[i - 1]);
		if (z_diff >= THRESHOLD)
			std::cout << "Z axis exceeded threshold\n";

	}

	iio_context_destroy(ctx);

	return 0;
}