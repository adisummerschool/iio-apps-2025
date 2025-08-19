#include <iostream>
#include <iio.h>
#include <errno.h>
#include <cstring>
#include <memory>
#include <vector>

#define URI "ip:10.76.84.233"
#define DEV_NAME "iio_ad5592r_s"
#define SAMPLE_COUNT32 32
#define THRESHOLD 100
#define SAMPLE_FREQUENCY 50

typedef struct main
{
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
        if (!ctx)
        {
                printf("Vezi ba ca nu merge contextul , eroare %s\n", strerror(errno));
                return -errno;
        }

        // Modify sample freq
        struct iio_device *trigger = iio_context_find_device(ctx, "trigger0");
        if (!trigger)
        {
                printf("Eroare trigger\n");
                return 1;
        }

        int ret = iio_device_attr_write(trigger, "sampling_frequency", std::to_string(SAMPLE_FREQUENCY).c_str());
        if (ret < 0)
        {
                std::cerr << "Nu am putut seta frecventa de esantionare, errno=" << errno << std::endl;
                return ret;
        }

        struct iio_device *dev = iio_context_find_device(ctx, DEV_NAME);
        if (!dev)
        {
                printf("Cam naspa nuemele asta de device\n");
                return -1;
        }

        for (int i = 0; i < 6; i++)
        {
                switch (i)
                {
                case 0:

                        channels[i] = iio_device_find_channel(dev, "voltage0", false);
                        iio_channel_enable(channels[i]);
                        if (!channels[i])
                        {
                                printf("Ai gresit ceva idk\n");
                                return -1;
                        }
                        else
                        {
                                double val;
                                int res = iio_channel_attr_read_double(channels[i], "raw", &val);
                                printf("Valoarea y-: %f \n", val);
                        }
                        break;

                case 1:

                        channels[i] = iio_device_find_channel(dev, "voltage1", false);
                        iio_channel_enable(channels[i]);
                        if (!channels[i])
                        {
                                printf("Ai gresit ceva idk\n");
                                return -1;
                        }
                        else
                        {
                                double val;
                                int res = iio_channel_attr_read_double(channels[i], "raw", &val);
                                printf("Valoarea y+: %f \n", val);
                        }
                        break;

                case 2:

                        channels[i] = iio_device_find_channel(dev, "voltage2", false);
                        iio_channel_enable(channels[i]);
                        if (!channels[i])
                        {
                                printf("Ai gresit ceva idk\n");
                                return -1;
                        }
                        else
                        {
                                double val;
                                int res = iio_channel_attr_read_double(channels[i], "raw", &val);
                                printf("Valoarea x+: %f \n", val);
                        }
                        break;

                case 3:

                        channels[i] = iio_device_find_channel(dev, "voltage3", false);
                        iio_channel_enable(channels[i]);
                        if (!channels[i])
                        {
                                printf("Ai gresit ceva idk\n");
                                return -1;
                        }
                        else
                        {
                                double val;
                                int res = iio_channel_attr_read_double(channels[i], "raw", &val);
                                printf("Valoarea x-: %f \n", val);
                        }
                        break;

                case 4:

                        channels[i] = iio_device_find_channel(dev, "voltage4", false);
                        iio_channel_enable(channels[i]);
                        if (!channels[i])
                        {
                                printf("Ai gresit ceva idk\n");
                                return -1;
                        }
                        else
                        {
                                double val;
                                int res = iio_channel_attr_read_double(channels[i], "raw", &val);
                                printf("Valoarea z+: %f \n", val);
                        }
                        break;

                case 5:

                        channels[i] = iio_device_find_channel(dev, "voltage5", false);
                        iio_channel_enable(channels[i]);
                        if (!channels[i])
                        {
                                printf("Ai gresit ceva idk\n");
                                return -1;
                        }
                        else
                        {
                                double val;
                                int res = iio_channel_attr_read_double(channels[i], "raw", &val);
                                printf("Valoarea z-: %f \n", val);
                        }
                        break;
                }
        }
        std::vector<int> x_axis(SAMPLE_COUNT32), y_axis(SAMPLE_COUNT32), z_axis(SAMPLE_COUNT32);
        iio_buffer *buf = iio_device_create_buffer(dev, SAMPLE_COUNT32, false);
        if (!buf)
        {
                std ::cerr << "Could not create buffer,errno" << errno << std::endl;
        }
        ssize_t bytes_read = iio_buffer_refill(buf);
        if (bytes_read < 0)
        {
                std ::cerr << "Could not refil,errno" << errno << std::endl;
        }
        ptrdiff_t step_size = iio_buffer_step(buf);
        for (uint16_t *sample = static_cast<uint16_t *>(iio_buffer_start(buf)); sample < iio_buffer_end(buf); sample += step_size)
        {
                current_samples current_sample;
                memcpy(&current_sample, sample, sizeof(current_sample));
                // current_sample.xpos = *sample;
                // current_sample.xneg = *(sample + 1);
                // current_sample.ypos = *(sample + 2);
                // current_sample.yneg = *(sample + 3);
                // current_sample.zpos = *(sample + 4);
                // current_sample.zneg = *(sample + 5);

                std::cout << current_sample.xpos << " " << current_sample.xneg << " "
                          << current_sample.ypos << " " << current_sample.yneg << " "
                          << current_sample.zpos << " " << current_sample.zneg << "\n";

                x_axis.push_back(current_sample.xpos - current_sample.xneg);
                y_axis.push_back(current_sample.ypos - current_sample.yneg);
                z_axis.push_back(current_sample.zpos - current_sample.zneg);
        }

        for (int i = 1; i < x_axis.size(); i++)
        {
                int x_diff = x_axis[i] - x_axis[i - 1];
                if (x_diff >= THRESHOLD)
                {
                        std ::cout << "X axis excedded threshlod \n";
                }
        }

        for (int i = 1; i < y_axis.size(); i++)
        {
                int y_diff = y_axis[i] - y_axis[i - 1];
                if (y_diff >= THRESHOLD)
                {
                        std ::cout << "Y axis excedded threshlod \n";
                }
        }

        for (int i = 1; i < z_axis.size(); i++)
        {
                int z_diff = z_axis[i] - z_axis[i - 1];
                if (z_diff >= THRESHOLD)
                {
                        std ::cout << "Z axis excedded threshlod \n";
                }
        }

        printf("Totul ok");
        return 0;
}