#include <stdio.h>
#include <fcntl.h>
#include <hw/i2c.h>
#include <ioctl.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include "aht10_util.h"

float _humidity = 0, _temperature = 0;

extern int fd_i2c1;

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
bool aht1x_begin(TwoWire *wire, int32_t sensor_id, uint8_t i2c_address)
{
	delay(20); // 20 ms to power up

	if (i2c_dev) {
		delete i2c_dev; // remove old interface
	}

	i2c_dev = new Adafruit_I2CDevice(i2c_address, wire);

	if (!i2c_dev->begin()) {
		return false;
	}

	uint8_t cmd[3];

	cmd[0] = AHTX0_CMD_SOFTRESET;
	if (!i2c_dev->write(cmd, 1)) {
		return false;
	}
	delay(20);

	while (getStatus() & AHTX0_STATUS_BUSY) {
		delay(10);
	}

	cmd[0] = AHTX0_CMD_CALIBRATE;
	cmd[1] = 0x08;
	cmd[2] = 0x00;
	i2c_dev->write(cmd, 3); // may not 'succeed' on newer AHT20s

	while (getStatus() & AHTX0_STATUS_BUSY) {
		delay(10);
	}
	if (!(getStatus() & AHTX0_STATUS_CALIBRATED)) {
		return false;
	}

	delete humidity_sensor;
	 
	
	temp_sensor = new Adafruit_AHTX0_Temp(this);
	return true;
}

/**
 * @brief  Gets the status (first byte) from AHT10/AHT20
 *
 * @returns 8 bits of status data, or 0xFF if failed
 */
uint8_t aht1x_getStatus(void)
{
	uint8_t ret;
	if (!i2c_dev->read(&ret, 1)) {
		return 0xFF;
	}
	return ret;
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
bool aht1x_getEvent(sensors_event_t *humidity, sensors_event_t *temp)
{
	uint32_t t = millis();

	// read the data and store it!
	uint8_t cmd[3] = {AHTX0_CMD_TRIGGER, 0x33, 0};
	if (!i2c_dev->write(cmd, 3)) {
		return false;
	}

	while (getStatus() & AHTX0_STATUS_BUSY) {
		delay(10);
	}

	uint8_t data[6];
	if (!i2c_dev->read(data, 6)) {
		return false;
	}
	uint32_t h = data[1];
	h <<= 8;
	h |= data[2];
	h <<= 4;
	h |= data[3] >> 4;
	_humidity = ((float)h * 100) / 0x100000;

	uint32_t tdata = data[3] & 0x0F;
	tdata <<= 8;
	tdata |= data[4];
	tdata <<= 8;
	tdata |= data[5];
	_temperature = ((float)tdata * 200 / 0x100000) - 50;

	// use helpers to fill in the events
	if (temp)
		fillTempEvent(temp, t);
	if (humidity)
		fillHumidityEvent(humidity, t);
	return true;
}
