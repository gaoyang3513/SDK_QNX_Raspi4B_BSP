#include <malloc.h>
#include <stdio.h>
#include <fcntl.h>
#include <hw/i2c.h>
#include <ioctl.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include "aht10_util.h"

static int fd_i2c;

static int _i2c_read(uint8_t address, uint8_t *data, size_t length, uint8_t stop)
{
	int ret = 0;
	char *data_recv = NULL;
	unsigned int recv_lent = sizeof(i2c_recv_t) + length;
	i2c_recv_t *data_msg = NULL, *data_buf = NULL;

	data_recv = malloc(recv_lent);
	if (data_recv == NULL) {
		printf("Failed to allocate memory for I2C read\n");
		return -1;
	}

	memset(data_recv, 0, recv_lent);

	data_msg = (i2c_recv_t *)data_recv;
	data_buf = (char *)data_recv + sizeof(*data_msg);

	data_msg->slave.addr = address;
	data_msg->slave.fmt  = I2C_ADDRFMT_7BIT;
	data_msg->len        = length;
	data_msg->stop       = stop;

	ret = devctl(fd_i2c, DCMD_I2C_RECV, data_msg, recv_lent, NULL);
	if (ret < 0) {
		printf("[%s|%u] ErrNo(%d) %s, failed to read I2C data\n", __FILE__, __LINE__, errno, strerror(errno));
		return ret;
	}

	memcpy(data, data_buf, length);

	return ret;
}

static int _i2c_write(uint8_t address, const uint8_t *data, size_t length, uint8_t stop)
{
	int ret = 0;
	char *data_send = NULL;
	i2c_send_t *data_msg = NULL;
	unsigned int send_lent = sizeof(i2c_send_t) + length;

	data_send = malloc (send_lent);
	if (data_send == NULL) {
		printf("Failed to allocate memory for I2C write\n");
		return -ENOMEM;
	}

	memset(data_send, 0, send_lent);

	data_msg = (i2c_send_t *)data_send;
	data_msg->slave.addr = address;
	data_msg->slave.fmt  = I2C_ADDRFMT_7BIT;
	data_msg->len        = length;
	data_msg->stop       = stop;

	memcpy ((char *)data_send + sizeof(*data_msg), data, length);

	ret = devctl(fd_i2c, DCMD_I2C_SEND, data_send, send_lent, NULL);
	if (ret < 0) {
		printf("[%12s|%4u] ErrNo(%d) %s, failed to write I2C data\n", __FILE_NAME__, __LINE__, errno, strerror(errno));
		return ret;
	}

	return ret;
}

/**
 * @brief  Gets the status (first byte) from AHT10/AHT20
 *
 * @returns 8 bits of status data, or 0xFF if failed
 */
static int aht1x_getStatus(uint8_t address)
{
	int ret;
	uint8_t recv_data[1] = {0};

	ret = _i2c_read(address, recv_data, 1, 1);
	if (ret < 0) {
		printf("[%s|%4u] ErrNo(%d) %s, failed to read date from I2C Address[%#X]\n", __FILE_NAME__, __LINE__, ret, address);
		return ret;
	}

	return recv_data[0];
}

/*!
 *    @brief  Sets up the hardware and initializes I2C
 *    @param  wire
 *            The Wire object to be used for I2C connections.
 *    @param  sensor_id
 *            The unique ID to differentiate the sensors from others
 *    @param  i2c_address
 *            The I2C address used to communicate with the sensor
 *    @return True if initialization was successful, otherwise false.
 */
int aht1x_begin(int i2c_dev, uint8_t i2c_address)
{
	int ret = 0;
	uint8_t cmd[3] = {0};

	if (i2c_dev < 0) {
		printf("Error, invalid I2C device\n");
		return -EINVAL;
	}
	fd_i2c = i2c_dev;

	cmd[0] = AHTX0_CMD_SOFTRESET;
	ret = _i2c_write(i2c_address, cmd, 1, 0);
	if (ret < 0) {
		printf("[%12s|%4u] ErrNo(%d), failed to write SOFTRESET command\n", __FILE_NAME__, __LINE__, ret);
		return ret;
	}

	usleep(20000); // 20ms delay

	while (aht1x_getStatus(i2c_address) & AHTX0_STATUS_BUSY) {
		usleep(10000);
	}

	cmd[0] = AHTX0_CMD_CALIBRATE;
	cmd[1] = 0x08;
	cmd[2] = 0x00;
	ret = _i2c_write(i2c_address, cmd, 3, 1);
	if (ret < 0) {
		printf("ErrNo(%d) %s, failed to write CALIBRATE command\n", errno, strerror(errno));
		return false;
	}

	while (aht1x_getStatus(i2c_address) & AHTX0_STATUS_BUSY) {
		delay(10);
	}
	if (!(aht1x_getStatus(i2c_address) & AHTX0_STATUS_CALIBRATED)) {
		return false;
	}

	return true;
}

/**************************************************************************/
/*!
    @brief  Gets the humidity sensor and temperature values as sensor events
    @param  humidity Sensor event object that will be populated with humidity
   data
    @param  temp Sensor event object that will be populated with temp data
    @returns true if the event data was read successfully
*/
/**************************************************************************/
int aht1x_getEvent(int i2c_dev, uint8_t i2c_address, float *_humidity, float *_temperature)
{
	int ret = 0;
	uint8_t retry = 0;
	uint8_t data[6] = {0};
	uint8_t cmd[3] = {AHTX0_CMD_TRIGGER, 0x33, 0};

	// read the data and store it!
	ret = _i2c_write(i2c_address, cmd, sizeof(cmd), 1);
	if (ret < 0) {
		printf("[%s|%4u] ErrNo(%d) %s, failed to write TRIGGER command\n", __FILE_NAME__, __LINE__, ret);
		return ret;
	}

	do {
		memset(data, 0, sizeof(data));

		ret = _i2c_read(i2c_address, data, sizeof(data), 1);
		if (ret < 0) {
			printf("[%s|%4u] ErrNo(%d) %s, failed to read date from I2C Address[%#X]\n", __FILE_NAME__, __LINE__, ret, i2c_address);
			return ret;
		}

		if (retry)
			usleep(200000);
	} while ((data[0] & AHTX0_STATUS_BUSY) && (retry++ < 10));

//	printf("Raw data: %02X %02X %02X %02X %02X %02X\n", data[0], data[1], data[2], data[3], data[4], data[5]);

	uint32_t h = (data[1] << 8 | data[2]) << 4 | data[3] >> 4;
	*_humidity = ((float)h * 100) / 0x100000;

	uint32_t tdata = ((data[3] & 0x0F)) << 16 | (data[4] << 8) | data[5];
	*_temperature = ((float)tdata * 200 / 0x100000) - 50;

	return true;
}
