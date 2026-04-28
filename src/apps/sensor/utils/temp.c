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

int temp_get(int chnl, float *temperature)
{
	int ret = 0, fd_th = 0;
	struct th_data th_data;

	fd_th = open ("/dev/th0", O_RDWR);
	if (fd_th < 0) {
		printf("ErrNo(%d) %s, failed to open I2C device\n", errno, strerror(errno));

		return -1;
	}

	ret = devctl(fd_th, DCMD_TH_GET_DATA, &th_data, sizeof(th_data), NULL);
	if (ret < 0) {
		printf("[%12s|%4u] ErrNo(%d), failed to read data from AHT1x\n", __FILE_NAME__, __LINE__, ret);
		return ret;
	}


	close(fd_th);

	return 0;
}
