#include <errno.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/iofunc.h>
#include <sys/dispatch.h>
#include <fcntl.h>
#include <ioctl.h>
#include <stdbool.h>
#include <hw/i2c.h>
#include "aht10_util.h"

static float humidity, temp;
static iofunc_attr_t            attr;
static resmgr_io_funcs_t        io_funcs;
static resmgr_connect_funcs_t   connect_funcs;

static char                     buffer[255] = "Hello world\n";

int get_event()
{
	int ret = 0, fd_i2c_s = 0;

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

//	while(1) {
		humidity =0.0f; temp = 0.0f;
		ret = aht1x_getEvent(fd_i2c_s, AHTX0_I2CADDR_DEFAULT, &humidity, &temp);
		if (ret < 0) {
			printf("[%12s|%4u] ErrNo(%d), failed to read data from AHT1x\n", __FILE_NAME__, __LINE__, ret);
			return ret;
		}

//		printf("Temperature: %2.3f ℃\n", temp);
//		printf("Humidity   : %2.3f %%rH\n", humidity);
//
//		sleep(1); // wait 500ms before next read
//	}

	close(fd_i2c_s);

	return 0;
}

int io_read (resmgr_context_t *ctp, io_read_t *msg, RESMGR_OCB_T *ocb)
{
    size_t      nleft;
    size_t      nbytes;
    size_t      offset = 0;
    int         nparts;
    int         status;

    if ((status = iofunc_read_verify (ctp, msg, ocb, NULL)) != EOK)
        return (status);

    if ((msg->i.xtype & _IO_XTYPE_MASK) != _IO_XTYPE_NONE)
        return (ENOSYS);

    get_event();

    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "Temperature: %2.3f ℃  \n", temp);
    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "Humidity   : %2.3f %%rH\n", humidity);

    /*
     *  On all reads (first and subsequent), calculate how many bytes we can
     *  return to the client, based upon the number of bytes available (nleft)
     *  and the client's buffer size
     */
    nleft = ocb->attr->nbytes - ocb->offset;
    nbytes = min (_IO_READ_GET_NBYTES(msg), nleft);

    if (nbytes > 0) {
        /* set up the return data IOV */
        SETIOV (ctp->iov, buffer + ocb->offset, nbytes);

        /* set up the number of bytes (returned by client's read()) */
        _IO_SET_READ_NBYTES (ctp, nbytes);

        /*
         * advance the offset by the number of bytes returned to the client
         */
        ocb->offset += nbytes;

        nparts = 1;
    } else {
        /*
         * they've asked for zero bytes or they've already previously
         * read everything
         */
        _IO_SET_READ_NBYTES (ctp, 0);

        nparts = 0;
    }

    /* mark the access time as invalid (we just accessed it) */
    if (msg->i.nbytes > 0)
        ocb->attr->flags |= IOFUNC_ATTR_ATIME;

    return (_RESMGR_NPARTS (nparts));
}


int main(int argc, char **argv)
{
    /* declare variables we'll be using */
    resmgr_attr_t        resmgr_attr;
    dispatch_t           *dpp;
    dispatch_context_t   *ctp;
    int                  id;

    /* initialize dispatch interface */
    if((dpp = dispatch_create()) == NULL) {
        fprintf(stderr, "%s: Unable to allocate dispatch handle.\n", argv[0]);
        return EXIT_FAILURE;
    }

    /* initialize resource manager attributes */
    memset(&resmgr_attr, 0, sizeof resmgr_attr);
    resmgr_attr.nparts_max = 1;
    resmgr_attr.msg_max_size = 2048;

    /* initialize functions for handling messages, including our read handlers */
    iofunc_func_init(_RESMGR_CONNECT_NFUNCS, &connect_funcs,
                     _RESMGR_IO_NFUNCS, &io_funcs);
    io_funcs.read   = io_read;
    io_funcs.read64 = io_read;

    /* initialize attribute structure used by the device */
    iofunc_attr_init(&attr, S_IFNAM | 0666, 0, 0);
    attr.nbytes = sizeof(buffer);

    /* attach our device name */
    if((id = resmgr_attach(dpp, &resmgr_attr, "/dev/th0", _FTYPE_ANY, 0,
                           &connect_funcs, &io_funcs, &attr)) == -1) {
        fprintf(stderr, "%s: Unable to attach name.\n", argv[0]);
        return EXIT_FAILURE;
    }

    /* allocate a context structure */
    ctp = dispatch_context_alloc(dpp);

    /* start the resource manager message loop */
    while(1) {
        if((ctp = dispatch_block(ctp)) == NULL) {
            fprintf(stderr, "block error\n");
            return EXIT_FAILURE;
        }
        dispatch_handler(ctp);
    }

    return EXIT_SUCCESS;
}
