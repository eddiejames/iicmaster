#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <err.h>
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <getopt.h>
#include <string.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

const char *helpstr =
	"Usage: i2cm [OPTION]\n"
	"-r <size>	read <size> bytes\n"
	"-w <size>	write <size> bytes\n"
	"-b <path>	open <path> bus for operations\n"
	"-a <addr>	use i2c device <addr>\n"
	"-o <offset>	use <offset> from start of device\n"
	"-d <data>	write <data>\n"
	"-W		use 2 bytes for offset\n"
	"\n"
	"Example: i2cm -b /dev/i2c-100 -a 0xa0 -W -o 0 -r 16\n"
	"reads 16 bytes from offset 0 (using 2 byte offset, as required by\n"
	"the device) of device 0xa0 on /dev/i2c-100 (corresponds to port 0\n"
	"of i2c master on proc 1)\n"
	"Example: i2cm -b /dev/i2c-201 -a 0xa8 -W -o 0 -d c0ffee00 -w 4\n"
	"writes 4 bytes (0xC0FFEE00) to offset 0 (using 2 byte offset) of\n"
	"device 0xa8 on /dev/i2c-201. (corresponds to port 1 of i2c master\n"
	"on proc 2)\n";

void get_data(uint8_t *buf, unsigned long size, char *arg)
{
	unsigned int i, count = strlen(arg);
	char c;
	uint8_t n;

	for (i = 0; i < count && (i / 2) < size; ++i) {
		c = arg[i];

		if (c >= '0' && c <= '9')
			n = c - '0';
		else if (c >= 'a' && c <= 'f')
			n = c - 'a' + 10;
		else if (c >= 'A' && c <= 'F')
			n = c - 'A' + 10;
		else
			n = 0;

		if (!(i % 2))
			buf[i / 2] = n << 4;
		else
			buf[i / 2] |= n;
	}
}

void display_buf(uint8_t *buf, unsigned long size)
{
	unsigned long i;

	for (i = 0; i < size; ++i) {
		printf("%02X ", buf[i]);
		if (!((i + 1) % 16))
			printf("\n");
	}
	if (i % 16)
		printf("\n");
}

int main(int argc, char **argv)
{
	char do_offset = 0;
	char doread = 1;
	char wide = 0;
	int rc = 0;
	char bus[32] = "/dev/i2c-100";
	int fd, option;
	uint8_t of1;
	uint16_t of2;
	unsigned long offset = 0;
	unsigned long addr = 0xa0;
	unsigned long size = 0;
	uint8_t *buf;
	char *data_arg = NULL;
	struct i2c_rdwr_ioctl_data rdwr;
	struct i2c_msg msgs[2];

	while ((option = getopt(argc, argv, "r:w:b:a:o:d:Wh")) != -1) {
		switch (option) {
		case 'r':
			doread = 1;
			errno = 0;
			size = strtoul(optarg, NULL, 0);
			if (size < 1 || errno) {
				printf("bad size arg %s\n", optarg);
				size = 0;
			}
			break;
		case 'w':
			doread = 0;
			errno = 0;
			size = strtoul(optarg, NULL, 0);
			if (size < 1 || errno) {
				printf("bad size arg %s\n", optarg);
				size = 0;
			}
			break;
		case 'b':
			strncpy(bus, optarg, 32);
			break;
		case 'a':
			errno = 0;
			addr = strtoul(optarg, NULL, 0);
			if (errno) {
				printf("bad addr arg %s\n", optarg);
				addr = 0xa0;
			}
			break;
		case 'o':
			errno = 0;
			offset = strtoul(optarg, NULL, 0);
			if (errno) {
				printf("bad offset arg %s\n", optarg);
				offset = 0;
			}
			else
				do_offset = 1;
			break;
		case 'd':
			data_arg = malloc(strlen(optarg));
			strcpy(data_arg, optarg);
			break;
		case 'W':
			wide = 1;
			break;
		case 'h':
			printf("%s", helpstr);
			break;
		default:
			printf("unknown option!\n\n%s", helpstr);
		}
	}

	if (!size)
		return -EINVAL;

	fd = open(bus, O_RDWR);
	if (fd < 0) {
		printf("failed to open %s\n", bus);
		return errno * -1;
	}

	buf = malloc(size + 2);
	if (!buf) {
		printf("failed to alloc mem\n");
		close(fd);
		return -ENOMEM;
	}

	memset(buf, 0, size + 2);

	if (data_arg) {
		if (!doread) {
			if (do_offset) {
				if (offset > 0xFF || wide) {
					of2 = __builtin_bswap16(offset);
					memcpy(buf, &of2, 2);
					get_data(buf + 2, size, data_arg);
				} else {
					of1 = offset;
					memcpy(buf, &of1, 1);
					get_data(buf + 1, size, data_arg);
				}
			} else
				get_data(buf, size, data_arg);
		}

		free(data_arg);
	}

	rc = ioctl(fd, I2C_TENBIT, 1);
	if (rc) {
		printf("failed to set 10 bit %d %d\n", rc, errno);
	}

	rc = ioctl(fd, I2C_SLAVE, addr & 0xFF);
	if (rc) {
		printf("failed to set device addr %d %d\n", rc, errno);
	}

	if (do_offset && doread) {
		msgs[0].addr = addr & 0xFF;
		msgs[0].flags = I2C_M_TEN;
		if (offset > 0xFF || wide) {
			of2 = __builtin_bswap16(offset);
			msgs[0].len = 2;
			msgs[0].buf = (uint8_t *)&of2;
		} else {
			of1 = offset;
			msgs[0].len = 1;
			msgs[0].buf = &of1;
		}

		msgs[1].addr = addr & 0xFF;
		msgs[1].flags = I2C_M_TEN | I2C_M_STOP | I2C_M_RD;
		msgs[1].len = size;
		msgs[1].buf = buf;

		rdwr.nmsgs = 2;
		rdwr.msgs = msgs;

		rc = ioctl(fd, I2C_RDWR, &rdwr);
		if (rc)
			printf("failed to read/write w/offset %d %d\n", rc,
			       errno);

		if (doread)
			printf("read:\n");
		else
			printf("wrote:\n");

		display_buf(buf, size);

		goto done;
	}

	if (doread) {
		rc = read(fd, buf, size);
		printf("read:\n");
	}
	else {
		rc = write(fd, buf, size + (do_offset ? ((offset > 0xFF || wide) ? 2 : 1) : 0));
		printf("wrote:\n");
	}

	if (do_offset) {
		if (offset > 0xFF || wide)
			display_buf(buf + 2, size);
		else
			display_buf(buf + 1, size);
	} else
		display_buf(buf, size);

	if (rc < 0)
		printf("failed to read/write %d %d\n", rc, errno);

done:
	free(buf);
	close(fd);

	return rc;
}
