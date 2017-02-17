#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#define GEARVR_MAX_FAILURE	  5
#define GEARVR_CDEV	"/dev/hidraw0"

struct gearvr_sample_data {
	uint8_t n;
	uint16_t t;
	uint16_t cmd_id;
	uint16_t temperature;

	struct stat stat_buf;

	int32_t ax, ay, az, gx, gy, gz;
};

int gearvr_read(int fd)
{
	int ret;
	int err = 0;
	struct pollfd fds;
	uint8_t buf[100];

	fds.fd = fd;
	fds.events = POLLIN | POLLHUP | POLLERR;

	while (1) {
		int i;
		struct gearvr_sample_data odata;

		if (err >= GEARVR_MAX_FAILURE) {
			fprintf(stderr, "reached maximum attempts\n");
			return -1;
		}

		ret = poll(&fds, 1, 100);
		if (ret < 0)
			fprintf(stderr, "failed to poll\n");

		if (ret == 0)
			fprintf(stderr, "timed out\n");

		if (!(fds.revents & POLLIN))
			fprintf(stderr, "poll exited with error\n");

		if ((ret <= 0) || !(fds.revents & POLLIN))
			goto fail;

		ret = read(fd, buf, 100);
		if (ret < 0) {
			fprintf(stderr, "read() failed\n");
			goto fail;
		}

		odata.n = buf[0];
		odata.t = (buf[3] << 8) | buf[2];
		odata.cmd_id = (buf[5] << 8) | buf[4];
		odata.temperature = (buf[7] << 8) | buf[6];

		/*
		printf("[%u] Read %d bytes: %u data, cmd_id = %u, tempreature = %u\n",
			odata.t, ret, odata.n, odata.cmd_id, odata.temperature);
		*/

		for (i = 0; i < odata.n && i < 2; i++) {
			odata.ax = (buf[0 + 8 + 16 * i] << 13) | (buf[1 + 8 + 16 * i] << 5)
				| ((buf[2 + 8 + 16 * i] & 0xF8) >> 3);
			odata.ay = ((buf[2 + 8 + 16 * i] & 0x07) << 18) | (buf[3 + 8 + 16 * i] << 10)
				| (buf[4 + 8 + 16 * i] << 2) | ((buf[5 + 8 + 16 * i] & 0xC0) >> 6);
			odata.az = ((buf[5 + 8 + 16 * i] & 0x3F) << 15) | (buf[6 + 8 + 16 * i] << 7)
				| (buf[7 + 8 + 16 * i] >> 1);

			odata.gx = (buf[0 + 16 + 16 * i] << 13) | (buf[1 + 16 + 16 * i] << 5)
				| ((buf[2 + 16 + 16 * i] & 0xF8) >> 3);
			odata.gy = ((buf[2 + 16 + 16 * i] & 0x07) << 18) | (buf[3 + 16 + 16 * i] << 10)
				| (buf[4 + 16 + 16 * i] << 2) | ((buf[5 + 16 + 16 * i] & 0xC0) >> 6);
			odata.gz = ((buf[5 + 16 + 16 * i] & 0x3F) << 15) | (buf[6 + 16 + 16 * i] << 7)
				| (buf[7 + 16 + 16 * i] >> 1);

			//printf("acc [%u]: %8d %8d %8d\n", odata.n, odata.ax, odata.ay, odata.az);
			printf("gyr: %8d %8d %8d\n", odata.gx, odata.gy, odata.gz);
		}

		err = 0;
		continue;

fail:
		err++;
		perror("failed to read from device");
		sleep(1);
	}
}

int main (int argc, char *argv[])
{
	int ret;
	int fd;

	fd = open(GEARVR_CDEV, O_RDONLY);
	if (fd < 0) {
		perror("cannot open file");
		return -1;
	}

	ret = gearvr_read(fd);

	close(fd);

	return ret;
}
