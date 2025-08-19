#include <cerrno>
#include <cstdint>
#include <cstring>
#include <iio.h>
#include <iostream>
#include <memory>
#include <vector>

#define URI "ip:10.76.84.239"
#define DEV_NAME "ad5592r_s"
#define SAMPLE_COUNT 200
#define THRESHOLD 100
#define SAMPLE_FREQUENCY 100

typedef struct {
  uint16_t xpos;
  uint16_t xneg;

  uint16_t ypos;
  uint16_t yneg;

  uint16_t zpos;
  uint16_t zneg;
} current_samples;

int main(int argc, char **argv) {
  struct iio_context *ctx;
  struct iio_channel *channels[6];
  struct iio_device *dev;

  ctx = iio_create_context_from_uri(URI);
  if (!ctx) {
    std::cerr << "Did not create context\n";
    return 1;
  }

  // Modify sample frequency
  struct iio_device *trigger = iio_context_find_device(ctx, "trigger0");
  if (!trigger) {
    std::cerr << "Did not find trigger\n";
    return 1;
  }

  // YOU CAN DO THIS !!! :muscle:
  int res = iio_device_attr_write_double(trigger, "sampling_frequency",
                                         SAMPLE_FREQUENCY);
  if (res < 0) {
    std::cerr << "cannot set sampling\n";
  }

  dev = iio_context_find_device(ctx, DEV_NAME);
  if (!dev) {
    std::cerr << "Did not find device\n";
    return 1;
  }

  channels[0] = iio_device_find_channel(dev, "voltage0", false);
  channels[1] = iio_device_find_channel(dev, "voltage1", false);
  channels[2] = iio_device_find_channel(dev, "voltage2", false);
  channels[3] = iio_device_find_channel(dev, "voltage3", false);
  channels[4] = iio_device_find_channel(dev, "voltage4", false);
  channels[5] = iio_device_find_channel(dev, "voltage5", false);

  for (int i = 0; i < 6; ++i) {
    iio_channel_enable(channels[i]);
  }

  iio_buffer *buf = iio_device_create_buffer(dev, SAMPLE_COUNT, false);
  if (!buf) {
    std::cerr << "Could not create buffer, errno: " << errno << std::endl;
  }

  ssize_t bytes_read = iio_buffer_refill(buf);
  if (bytes_read < 0) {
    std::cerr << "Could not refill buffer, errno: " << bytes_read << std::endl;
  }

  std::vector<int> x_axis, y_axis, z_axis;

  ptrdiff_t step_size = iio_buffer_step(buf);
  int i = 0;
  for (uint16_t *sample = static_cast<uint16_t *>(iio_buffer_start(buf));
       sample < iio_buffer_end(buf); sample += step_size) {

    current_samples current_sample;
    memcpy(&current_sample, sample, sizeof(current_samples));
    // current_sample.xpos = *sample;
    // current_sample.xneg = *(sample + 1);
    // current_sample.ypos = *(sample + 2);
    // current_sample.yneg = *(sample + 3);
    // current_sample.zpos = *(sample + 4);
    // current_sample.zneg = *(sample + 5);

    // std::cout << i++ << ": " << current_sample.xpos << " "
    //           << current_sample.xneg << " " << current_sample.ypos << " "
    //           << current_sample.yneg << " " << current_sample.zpos << " "
    //           << current_sample.zneg << "\n";

    x_axis.push_back(current_sample.xpos - current_sample.xneg);
    y_axis.push_back(current_sample.ypos - current_sample.yneg);
    z_axis.push_back(current_sample.zpos - current_sample.zneg);
  }

  for (size_t i = 1; i < x_axis.size(); ++i) {
    int x_diff = abs(x_axis[i] - x_axis[i - 1]);
    if (x_diff >= THRESHOLD) {
      std::cout << "X axis exceeded threshold\n";
    }

    int y_diff = abs(y_axis[i] - y_axis[i - 1]);
    if (y_diff >= THRESHOLD) {
      std::cout << "Y axis exceeded threshold\n";
    }

    int z_diff = abs(z_axis[i] - z_axis[i - 1]);
    if (z_diff >= THRESHOLD) {
      std::cout << "Z axis exceeded threshold\n";
    }
  }

  return 0;
}
