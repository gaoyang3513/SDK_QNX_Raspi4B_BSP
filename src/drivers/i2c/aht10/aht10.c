#include <stdio.h>
#include <fcntl.h>
#include <hw/i2c.h>
#include <ioctl.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include "aht10_util.h"

int fd_i2c1 = -1;

extern bool aht1x_getEvent(sensors_event_t *humidity, sensors_event_t *temp);

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


	Adafruit_AHTX0 aht;

	Serial.begin(115200);
	Serial.println("Adafruit AHT10/AHT20 demo!");

	if (! aht.begin()) {
		Serial.println("Could not find AHT? Check wiring");
		while (1) delay(10);
	}
	
	Serial.println("AHT10 or AHT20 found");

	while(1) {
		sensors_event_t humidity, temp;
		aht.getEvent(&humidity, &temp);// populate temp and humidity objects with fresh data
		Serial.print("Temperature: "); Serial.print(temp.temperature); Serial.println(" degrees C");
		Serial.print("Humidity: "); Serial.print(humidity.relative_humidity); Serial.println("% rH");

		delay(500);
	}

	close(fd_i2c_s);

	return 0;
}