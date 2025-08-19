#include "matplotlibcpp.h"
#include <iostream>
#include <iio.h>
#include <cstring>
#include <vector>

#define URI "ip:10.76.84.230"
#define DEV_NAME "iio-adc-ad5592"
#define SAMPLE_COUNT 32
#define THRESHOLD 100
#define SAMPLE_FREQUENCY 50

namespace plt = matplotlibcpp;

typedef struct {
        uint16_t xpos;
        uint16_t xneg;
        uint16_t ypos;
        uint16_t yneg;
        uint16_t zpos;
        uint16_t zneg;
} sample_buffer_t;

int main(int argc, char **argv){
        struct iio_channel *channels[6];
        struct iio_context *ctx = iio_create_context_from_uri(URI);
        if (!ctx) {
                std::cerr << "Failed to create IIO context" << std::endl;
                return -1;
        }

        struct iio_device *trigger = iio_context_find_device(ctx, "trigger0");
        if (!trigger) {
                printf("Failed to find IIO device for trigger0\n");
                iio_context_destroy(ctx);
                return -1;
        }

        double frequency;
        if (iio_device_attr_read_double(trigger, "sampling_frequency", &frequency) < 0) {
                printf("Failed to read IIO attribute frequency\n");
                iio_context_destroy(ctx);
                return -1;
        }

        struct iio_device *dev = iio_context_find_device(ctx, DEV_NAME);
        if (!dev) {
                std::cerr << "Failed to find IIO device" << std::endl;
                iio_context_destroy(ctx);
                return -1;
        }

        channels[0] = iio_device_find_channel(dev, "voltage0", false); // x+
        channels[1] = iio_device_find_channel(dev, "voltage1", false); // x-
        channels[2] = iio_device_find_channel(dev, "voltage3", false); // y-
        channels[3] = iio_device_find_channel(dev, "voltage2", false); // y+
        channels[4] = iio_device_find_channel(dev, "voltage5", false); // z-
        channels[5] = iio_device_find_channel(dev, "voltage4", false); // z+

        const char* channel_names[6] = {"x+", "x-", "y-", "y+", "z-", "z+"};
        for (int i = 0; i < 6; ++i) {
                if (!channels[i]) {
                        std::cerr << "Failed to find IIO channel " << channel_names[i] << std::endl;
                        return -1;
                }
                iio_channel_enable(channels[i]);
        }

        std::cout << "Successfully found all IIO channels" << std::endl;

        iio_buffer *buffer = iio_device_create_buffer(dev, SAMPLE_COUNT, false);
        if (!buffer) {
                std::cerr << "Failed to create IIO buffer" << std::endl;
                return -1;
        }

        ssize_t bytes_read = iio_buffer_refill(buffer);
        if (bytes_read < 0) {
                std::cerr << "Failed to read IIO buffer" << std::endl;
                return -1;
        }

        int count = 0;
        std::vector<int> x_axis, y_axis, z_axis, t_axis;

                ptrdiff_t step_size = iio_buffer_step(buffer);
                uint16_t *buf_start = static_cast<uint16_t *>(iio_buffer_start(buffer));
                uint16_t *buf_end = static_cast<uint16_t *>(iio_buffer_end(buffer));
                size_t sample_size = sizeof(sample_buffer_t);
                for (uint16_t *ptr = buf_start; (uint8_t*)ptr + sample_size <= (uint8_t*)buf_end; ptr += step_size/sizeof(uint16_t)) {
                        sample_buffer_t sample_buffer;
                        memcpy(&sample_buffer, ptr, sizeof(sample_buffer_t));
                        x_axis.push_back(sample_buffer.xpos - sample_buffer.xneg);
                        y_axis.push_back(sample_buffer.ypos - sample_buffer.yneg);
                        z_axis.push_back(sample_buffer.zpos - sample_buffer.zneg);
                        t_axis.push_back(count++);
                }


        for (size_t i = 0; i < x_axis.size(); ++i) {
                std::cout << "Sample " << i << ": x = " << x_axis[i]
                          << ", y = " << y_axis[i] << ", z = " << z_axis[i] << std::endl;
        }

        // for (size_t i = 1; i < x_axis.size(); ++i) {
        //         int x_diff = abs(x_axis[i] - x_axis[i - 1]);
        //         if (x_diff > THRESHOLD) {
        //                 std::cout << "X-axis spike detected between samples " << i - 1 << " and " << i << std::endl;
        //         }
        // }

        // for (size_t i = 1; i < y_axis.size(); ++i) {
        //         int y_diff = abs(y_axis[i] - y_axis[i - 1]);
        //         if (y_diff > THRESHOLD) {
        //                 std::cout << "Y-axis spike detected between samples " << i - 1 << " and " << i << std::endl;
        //         }
        // }

        // for (size_t i = 1; i < z_axis.size(); ++i) {
        //         int z_diff = abs(z_axis[i] - z_axis[i - 1]);
        //         if (z_diff > THRESHOLD) {
        //                 std::cout << "Z-axis spike detected between samples " << i - 1 << " and " << i << std::endl;
        //         }
        // }

        plt::figure_size(1000,600);
        plt::named_plot("X", t_axis, x_axis, "b-");
        plt::named_plot("Y", t_axis, y_axis, "r-");
        plt::named_plot("Z", t_axis, z_axis, "g-");
        plt::xlabel("Time");
        plt::ylabel("Acc");
        plt::legend();
        plt::show();
        return 0;
}