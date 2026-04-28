#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <hw/i2c.h>
#include <devctl.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <sys/iofunc.h>
#include <sys/neutrino.h>
#include "bsp_th.h"
#include "utils_th.h"

int main()
{
	int ret = 0, fd_th = 0;
	float temp = 0.0f, humi = 0.0f;

	while(1) {
		ret = temp_get(0, &temp);
		if (ret < 0) {
			printf("[%12s|%4u] ErrNo(%d), failed to read data from AHT1x\n", __FILE_NAME__, __LINE__, ret);
			return ret;
		}

		ret = humi_get(0, &humi);
		if (ret < 0) {
			printf("[%12s|%4u] ErrNo(%d), failed to read data from AHT1x\n", __FILE_NAME__, __LINE__, ret);
			return ret;
		}

		printf("Temperature: %2.3f ℃\n",   temp / 1000.0f);
		printf("Humidity   : %2.3f %%rH\n", humi / 1000.0f);

		sleep(1); // wait 500ms before next read
	}

	close(fd_th);

	return 0;
}
