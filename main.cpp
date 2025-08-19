#include <iostream>
#include <iio.h>
#include <errno.h>
#include <cstring>
#include <vector>
#include <matplotlibcpp.h>

namespace plt = matplotlibcpp;

// #define URI                     "ip:10.76.84.194"
#define URI                     "ip:10.76.84.219"
#define DEV_NAME                "iio-ad5592r-s"
#define SAMPLE_COUNT            200
#define THRESHOLD               100
#define SAMPLE_FREQUENCY        100    

#define CHANNEL_PREFIX "voltage"
#define NUM_CHANNELS 6

typedef struct
{
        uint16_t xpos;
        uint16_t xneg;

        uint16_t ypos;
        uint16_t yneg;

        uint16_t zpos;
        uint16_t zneg;
} current_samples;

using namespace std;

int main(int argc, char **argv)
{
        iio_channel *channels[6];

        // create context
        iio_context *ctx = iio_create_context_from_uri(URI);
        if (!ctx)
        {
                cout << "Error creating IIO context " << URI << ": " << strerror(errno) << endl;
                return -errno;
        }

        struct iio_device *trigger = iio_context_find_device(ctx, "trigger0");
        if (!trigger)
        {
                std::cerr << "Error finding trigger device: " << strerror(errno) << std::endl;
                return 1;
        }

        double current_freq = 0;
        int res = iio_device_attr_read_double(trigger, "sampling_frequency", &current_freq);
        if (res) {
                std::cerr << "Error reading sampling_frequency: " << strerror(errno) << std::endl;
                return 1;
        }
        if (current_freq != SAMPLE_FREQUENCY) {
                iio_device_attr_write_double(trigger, "sampling_frequency", SAMPLE_FREQUENCY);
        }

        // find device
        iio_device *dev = iio_context_find_device(ctx, DEV_NAME);
        if (!dev)
        {
                cout << "Error finding device " << DEV_NAME << ": " << strerror(errno) << endl;
                return -errno;
        }

        // find channels
        for (int i = 0; i < NUM_CHANNELS; i++)
        {
                char channel_name[10] = CHANNEL_PREFIX;
                int name_len = strlen(channel_name);
                channel_name[name_len] = '0' + i;
                channel_name[name_len + 1] = '\0';

                channels[i] = iio_device_find_channel(dev, channel_name, false);
                if (!channels[i])
                {
                        cout << "Error finding channel " << channel_name << ": " << strerror(errno) << endl;
                        return -errno;
                }
        }

        /*
            // read channels
            for (int i = 0; i < NUM_CHANNELS; i++)
            {
                double val;
                int res = iio_channel_attr_read_double(channels[i], "raw", &val);
                if (res)
                {
                    cout << "Error reading channel " << i << ": " << strerror(errno) << endl;
                    return -errno;
                }
                cout << val << ' ';
            }
        */

        // enable channels
        for (int i = 0; i < NUM_CHANNELS; i++)
                iio_channel_enable(channels[i]);

        // create buffer
        iio_buffer *buf = iio_device_create_buffer(dev, SAMPLE_COUNT, false);
        if (!buf)
                cout << "Could not create buffer:" << strerror(errno) << endl;

        // refill buffer
        ssize_t bytes_read = iio_buffer_refill(buf);
        if (bytes_read < 0)
                cout << "Could not refill buffer:" << strerror(errno) << endl;

        cout << endl;
        cout << "All ok!" << endl;

        // Declare vectors to store axis data
        std::vector<int> x_axis;
        std::vector<int> y_axis;
        std::vector<int> z_axis;
        std::vector<int> t_axis;

        ptrdiff_t step_size = iio_buffer_step(buf);
        int count = 0;

        for (uint16_t *sample = static_cast<uint16_t *>(iio_buffer_start(buf)); sample < iio_buffer_end(buf); sample += step_size)
        {
                current_samples current_sample;
                current_sample.xpos = *sample;
                current_sample.xneg = *(sample + 1);

                current_sample.ypos = *(sample + 2);
                current_sample.yneg = *(sample + 3);

                current_sample.zpos = *(sample + 4);
                current_sample.zneg = *(sample + 5);

                cout << current_sample.xpos << ' ' << current_sample.xneg << ' '
                     << current_sample.ypos << ' ' << current_sample.yneg << ' '
                     << current_sample.zpos << ' ' << current_sample.zneg << endl;

                x_axis.push_back(current_sample.xpos - current_sample.xneg);
                y_axis.push_back(current_sample.ypos - current_sample.yneg);
                z_axis.push_back(current_sample.zpos - current_sample.zneg);
                t_axis.push_back(count++);
        }


        // Analyze the data for significant changes, such as shocks
        // for (int i = 1; i < x_axis.size(); ++i) {
        //         int x_diff = x_axis[i] - x_axis[i - 1];
        //         if (x_diff > THRESHOLD) {
        //                 cout << "X-axis significant change detected: " << x_diff << endl;
        //         }

        //         int y_diff = y_axis[i] - y_axis[i - 1];
        //         if (y_diff > THRESHOLD) {
        //                 cout << "Y-axis significant change detected: " << y_diff << endl;
        //         }

        //         int z_diff = z_axis[i] - z_axis[i - 1];
        //         if (z_diff > THRESHOLD) {
        //                 cout << "Z-axis significant change detected: " << z_diff << endl;
        //         }
        // }

        plt::figure_size(1000, 600);
        plt::named_plot("X-axis", t_axis, x_axis, "b-");
        plt::named_plot("Y-axis", t_axis, y_axis, "r-");
        plt::named_plot("Z-axis", t_axis, z_axis, "g-");
        plt::xlabel(std::string("Time"));
        plt::ylabel(std::string("Accelerometer axis Values"));
        plt::legend();
        plt::show();

        // Exit:
        iio_context_destroy(ctx);
        return 0;
}