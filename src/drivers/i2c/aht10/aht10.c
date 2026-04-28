#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <sys/iofunc.h>
#include <sys/dispatch.h>
#include <string.h>

static resmgr_connect_funcs_t   connect_func;
static resmgr_io_funcs_t        io_func;
static iofunc_attr_t            attr;

int main (int argc, char **argv)
{
    thread_pool_attr_t    pool_attr;
    thread_pool_t         *tpp;
    dispatch_t            *dpp;
    resmgr_attr_t         resmgr_attr;
    int                   id;

    if ((dpp = dispatch_create ()) == NULL) {
        fprintf (stderr,"%s:  Unable to allocate dispatch context.\n", argv [0]);
        return (EXIT_FAILURE);
    }

    memset (&pool_attr, 0, sizeof (pool_attr));
    pool_attr.handle = dpp;
    pool_attr.context_alloc = (void *) dispatch_context_alloc;
    pool_attr.block_func    = (void *) dispatch_block;
    pool_attr.handler_func  = (void *) dispatch_handler;
    pool_attr.context_free  = (void *) dispatch_context_free;

    // 1) set up the number of threads that you want
    pool_attr.lo_water  = 2;
    pool_attr.hi_water  = 4;
    pool_attr.increment = 1;
    pool_attr.maximum   = 50;

    if ((tpp = thread_pool_create (&pool_attr, POOL_FLAG_EXIT_SELF)) == NULL) {
        fprintf (stderr, "%s:  Unable to initialize thread pool.\n", argv [0]);
        return (EXIT_FAILURE);
    }

    iofunc_func_init (_RESMGR_CONNECT_NFUNCS, &connect_func, _RESMGR_IO_NFUNCS, &io_func);
    iofunc_attr_init (&attr, S_IFNAM | 0777, 0, 0);

    // 2) override functions in "connect_func" and "io_func" as required here
    memset (&resmgr_attr, 0, sizeof (resmgr_attr));
    resmgr_attr.nparts_max   = 1;
    resmgr_attr.msg_max_size = 2048;

    // 3) replace "/dev/whatever" with your device name
    if ((id = resmgr_attach (dpp, &resmgr_attr, "/dev/whatever", _FTYPE_ANY, 0, &connect_func, &io_func, &attr)) == -1) {
        fprintf (stderr, "%s:  Unable to attach name.\n", argv [0]);
        return (EXIT_FAILURE);
    }

    // Never returns
    thread_pool_start (tpp);

    return (EXIT_SUCCESS);
}

/*
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
	float humidity, temp;

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
		humidity =0.0f; temp = 0.0f;
		ret = aht1x_getEvent(fd_i2c_s, AHTX0_I2CADDR_DEFAULT, &humidity, &temp);
		if (ret < 0) {
			printf("[%12s|%4u] ErrNo(%d), failed to read data from AHT1x\n", __FILE_NAME__, __LINE__, ret);
			return ret;
		}

		printf("Temperature: %2.3f ℃\n", temp);
		printf("Humidity   : %2.3f %%rH\n", humidity);

		sleep(1); // wait 500ms before next read
	}

	close(fd_i2c_s);

	return 0;
}
*/
