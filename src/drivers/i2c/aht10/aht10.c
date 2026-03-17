#include <stdio.h>
#include <fcntl.h>
#include <hw/i2c.h>
#include <ioctl.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include "aht10_util.h"

int main() {
	int ret = 0, fd_i2c_s = 0;
	i2c_driver_info_t info = {0};

	fd_i2c_s = open ("/dev/i2c1", O_RDWR);
	if (fd_i2c_s < 0) {
		printf("ErrNo(%d) %s, failed to open I2C device\n", errno, strerror(errno));
	
		return -1;
	}

	ret = aht1x_begin(fd_i2c_s, AHTX0_I2CADDR_DEFAULT);
	if (ret < 0) {
		printf("[%12s|%4u] ErrNo(%d), failed to init AHT1x\n", __FILE_NAME__, __LINE__, ret);
		close(fd_i2c_s);
		return -1;
	}
	
	while(1) {
		float humidity, temp;
		ret = aht1x_getEvent(fd_i2c_s, AHTX0_I2CADDR_DEFAULT, &humidity, &temp);
		if (ret < 0) {
			printf("[%12s|%4u] ErrNo(%d), failed to read data from AHT1x\n", __FILE_NAME__, __LINE__, ret);
			return ret;
		}

		printf("Temperature: %2.3f degrees C\n", temp);
		printf("Humidity   : %2.3f %% rH\n", humidity);

		usleep(500000); // wait 500ms before next read
	}

	close(fd_i2c_s);

	return 0;
}