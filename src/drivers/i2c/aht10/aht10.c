#include <stdio.h>
#include <fcntl.h>
#include <hw/i2c.h>
#include <ioctl.h>
#include <errno.h>
#include <string.h>

int main() {
    int ret = 0, fd_i2c_s = 0;
    i2c_driver_info_t info = {0};

    fd_i2c_s = open ("/dev/i2c1", O_RDWR);
    if (fd_i2c_s < 0) {
        printf("ErrNo(%d) %s, failed to open I2C device\n", errno, strerror(errno));
        return -1;
    }

    ret = ioctl(fd_i2c_s, DCMD_I2C_DRIVER_INFO, &info);
    if (ret < 0) {
        printf("ErrNo(%d) %s, failed to read from I2C device\n", errno, strerror(errno));
        close(fd_i2c_s);
        return -1;
    }

    printf("I2C Driver Info:\n");
    printf("  Speed    : %#X\n", info.speed_mode); /* supported speeds: I2C_SPEED_* */
    printf("  Mode-Addr: %#X\n", info.addr_mode);  /* supported address fmts: I2C_ADDRFMT_* */
    printf("  Verbosity: %#X\n", info.verbosity);  /* Driver verbosity level */

    close(fd_i2c_s);

    return 0;
}