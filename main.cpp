#include <iostream>
#include <iio.h>
#include <errno.h>
#include <cstring>
#include <vector>

#define URI "ip:10.76.84.219"
#define DEV_NAME "iio-ad5592r-s"
#define SAMPLE_COUNT 32

#define TRIG_NAME "trigger0"
#define SAMPLE_FREQ 50

#define CHANNEL_PREFIX "voltage"
#define NUM_CHANNELS 6

#define THRESHOLD 500

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

    // find device
    iio_device *dev = iio_context_find_device(ctx, DEV_NAME);
    if (!dev)
    {
        cout << "Error finding device " << DEV_NAME << ": " << strerror(errno) << endl;
        return -errno;
    }

    // adjust sample rate
    iio_device *tmr = iio_context_find_device(ctx, TRIG_NAME);
    if (!tmr)
    {
        cout << "Error finding trigger " << TRIG_NAME << ": " << strerror(errno) << endl;
        return -errno;
    }

    // read before write
    long long current_freq;
    int res = iio_device_attr_read_longlong(tmr, "sampling_frequency", &current_freq);
    if (res)
    {
        cout << "Error getting sample rate: " << strerror(errno) << endl;
        return -errno;
    }

    if (current_freq != SAMPLE_FREQ)
    {
        cout << "Adjusting sample rate from " << current_freq << " Hz to " << SAMPLE_FREQ << " Hz" << endl;
        res = iio_device_attr_write_longlong(tmr, "sampling_frequency", SAMPLE_FREQ);

        if (res)
        {
            cout << "Error getting sample rate: " << strerror(errno) << endl;
            return -errno;
        }
    }

    // find channels
    for (int i = 0; i < NUM_CHANNELS; i++)
    {
        char channel_name[10] = CHANNEL_PREFIX;
        int name_len = strlen(channel_name);
        channel_name[name_len] = '0' + i;
        channel_name[name_len + 1] = '\0';
        //  cout << channel_name << endl;

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
    {
        cout << "Could not create buffer:" << strerror(errno) << endl;
        return -errno;
    }

    // refill buffer
    ssize_t bytes_read = iio_buffer_refill(buf);
    if (bytes_read < 0)
    {
        cout << "Could not refill buffer:" << strerror(errno) << endl;
        return -errno;
    }
    cout << endl;
    cout << "All ok!" << endl;

    ptrdiff_t step_size = iio_buffer_step(buf);

    vector<int16_t> x_axis(SAMPLE_COUNT), y_axis(SAMPLE_COUNT), z_axis(SAMPLE_COUNT);

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
    }

    for (int i = 1; i < x_axis.size(); ++i)
    {
        int x_diff = abs(x_axis[i] - x_axis[i - 1]);
        if (x_diff >= THRESHOLD)
            cout << "șoc șoc șoc x" << endl;

        int y_diff = abs(y_axis[i] - y_axis[i - 1]);
        if (y_diff >= THRESHOLD)
            cout << "șoc șoc șoc y" << endl;

        int z_diff = abs(z_axis[i] - z_axis[i - 1]);
        if (z_diff >= THRESHOLD)
            cout << "șoc șoc șoc z" << endl;
    }

    // Exit:
    iio_context_destroy(ctx);
    return 0;
}