#include <iostream>
#include <stdio.h>
#include <iio.h>
#include <errno.h>
#include <string.h>
#include <string>
#include <stdio.h>
#include <vector>

#define URI "ip:10.76.84.215"
#define DEV_NAME "iio_ad5592"
#define SAMPLE_COUNT 32
#define THRESHOLD 100
#define SAMPLE_FREQUENCY 50

typedef struct {

        uint16_t xpos;
        uint16_t xneg;
        
        uint16_t ypos;
        uint16_t yneg;

        uint16_t zpos;
        uint16_t zneg;
        
} current_samples;

int main(int argc, char **argv) {

        struct iio_context* ctx = iio_create_context_from_uri(URI);
        if (!ctx) {
                printf("Nu merge contextul, eroare: %s\n", strerror(errno));
                return -errno;
        }

        struct iio_device *trigger = iio_context_find_device(ctx, "trigger0");
        if (!trigger) {
                printf("Trigger negasit\n");
                return -1;
        }

        double sampling_freq;
        int ok = iio_device_attr_read_double(trigger, "sampling_frequency", &sampling_freq);
        if (ok < 0) {
                printf("N-am putut extrage sample frequency\n");
                return -1;
        }
        if (sampling_freq != SAMPLE_FREQUENCY) {
                ok = iio_device_attr_write_double(trigger, "sampling_frequency", SAMPLE_FREQUENCY);
                if (ok < 0) {
                        printf("N-am putut scrie frecenta\n");
                        return -1;
                }
        }

        struct iio_device* dev = iio_context_find_device(ctx, DEV_NAME);
        if (!dev) {
                printf("Device negasit\n");
                return -1;
        }

        struct iio_channel *channels[6];
        for (int i = 0 ; i < 6; i++) {
                char str[1];
                char chan[10] = "voltage";
                char mes[50] = "Canal negasit: voltage";
                sprintf(str, "%d", i);
                channels[i] = iio_device_find_channel(dev, strcat(chan, str), false);
                if (!channels[i]) {
                        printf(strcat(strcat(mes, str), "\n"));
                        return -1;
                }
                iio_channel_enable(channels[i]);
        }

        iio_buffer *buf = iio_device_create_buffer(dev, SAMPLE_COUNT, false);
        if (!buf) {
                printf("Buffer-ul nu poate fi creat, eroare: %s\n", strerror(errno));
                return -errno;
        }

        ssize_t byte_read = iio_buffer_refill(buf);
        if(byte_read < 0) {
                printf("Buffer-ul nu-si ia refill, eroare: %s\n", strerror(errno));
                return -errno;
        }

        uint16_t step_size = iio_buffer_step(buf);
        current_samples current_sample;
        std::vector<int> x_axis(SAMPLE_COUNT), y_axis(SAMPLE_COUNT), z_axis(SAMPLE_COUNT);

        for (uint16_t *sample = static_cast<uint16_t *>(iio_buffer_start(buf)); 
                sample < iio_buffer_end(buf) ; sample += step_size) {

                        memcpy(&current_sample, sample, 6 * sizeof(uint16_t));

                        std::cout       << current_sample.xpos << " " << current_sample.xneg << " "
                                        << current_sample.ypos << " " << current_sample.yneg << " "
                                        << current_sample.zpos << " " << current_sample.zneg << "\n";

                        x_axis.push_back(current_sample.xpos - current_sample.xneg);
                        y_axis.push_back(current_sample.ypos - current_sample.yneg);
                        z_axis.push_back(current_sample.zpos - current_sample.zneg);
        }

        for (size_t i = 1; i < x_axis.size(); i++) {
                int x_diff = abs(x_axis[i] - x_axis[i-1]);
                if (x_diff >= THRESHOLD) {
                        std::cout << "X axis exceeded threshold\n";
                }
                int y_diff = abs(y_axis[i] - y_axis[i-1]);
                if (y_diff >= THRESHOLD) {
                        std::cout << "Y axis exceeded threshold\n";
                }
                int z_diff = abs(z_axis[i] - z_axis[i-1]);
                if (z_diff >= THRESHOLD) {
                        std::cout << "Z axis exceeded threshold\n";
                }
        }

        printf("Totul bine!\n");
        
        iio_context_destroy(ctx);
        return 0;
}