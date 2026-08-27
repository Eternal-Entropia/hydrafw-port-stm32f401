/*
HydraBus/HydraNFC - Copyright (C) 2014-2023 Benjamin VERNOUX

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
#include "bsp.h"
#include "bsp_i2c_master.h"
#include "bsp_i2c_conf.h"

#define I2C_SPEED_MAX (4)
static const uint32_t i2c_freqs[I2C_SPEED_MAX] = {
	50000,
	100000,
	400000,
	1000000
};
static int i2c_speed_delay;
static bool i2c_started;
static uint32_t i2c_clock_strech_timeout;

typedef struct {
	GPIO_TypeDef *scl_port;
	uint16_t scl_pin;
	GPIO_TypeDef *sda_port;
	uint16_t sda_pin;
} bsp_i2c_hw_t;

static const bsp_i2c_hw_t bsp_i2c_hw[BSP_DEV_I2C_END] = {
	/* I2C1 */
	{
		.scl_port = BSP_I2C1_SCL_PORT,
		.scl_pin = BSP_I2C1_SCL_PIN,
		.sda_port = BSP_I2C1_SDA_PORT,
		.sda_pin = BSP_I2C1_SDA_PIN,
	},
	/* I2C2 */
	{
		.scl_port = BSP_I2C2_SCL_PORT,
		.scl_pin = BSP_I2C2_SCL_PIN,
		.sda_port = BSP_I2C2_SDA_PORT,
		.sda_pin = BSP_I2C2_SDA_PIN,
	},
	/* I2C3 */
	{
		.scl_port = BSP_I2C3_SCL_PORT,
		.scl_pin = BSP_I2C3_SCL_PIN,
		.sda_port = BSP_I2C3_SDA_PORT,
		.sda_pin = BSP_I2C3_SDA_PIN,
	},
};

/* Set SCL LOW = 0/GND */
#define set_scl_low(dev)   (gpio_set_pin(bsp_i2c_hw[(dev)].scl_port, bsp_i2c_hw[(dev)].scl_pin))
/* Set SCL HIGH / Floating Input */
#define set_scl_float(dev) (gpio_clr_pin(bsp_i2c_hw[(dev)].scl_port, bsp_i2c_hw[(dev)].scl_pin))

/* Set SDA LOW = 0/GND */
#define set_sda_low(dev)   (gpio_set_pin(bsp_i2c_hw[(dev)].sda_port, bsp_i2c_hw[(dev)].sda_pin))
/* Set SDA HIGH / Floating Input */
#define set_sda_float(dev) (gpio_clr_pin(bsp_i2c_hw[(dev)].sda_port, bsp_i2c_hw[(dev)].sda_pin))

/* Get SDA pin state 0 or 1 */
#define get_sda(dev)       (gpio_get_pin(bsp_i2c_hw[(dev)].sda_port, bsp_i2c_hw[(dev)].sda_pin))

/* Get SCL pin state 0 or 1 */
#define get_scl(dev)       (gpio_get_pin(bsp_i2c_hw[(dev)].scl_port, bsp_i2c_hw[(dev)].scl_pin))

/* wait I2C half clock delay */
#define i2c_sw_delay()     (wait_delay(i2c_speed_delay))

/** \brief I2C SW Bit Banging GPIO HW DeInit.
 *
 * \param dev_num bsp_dev_i2c_t: I2C dev num
 * \return void
 *
 */
static void i2c_gpio_hw_deinit(bsp_dev_i2c_t dev_num)
{
	if(dev_num >= BSP_DEV_I2C_END) return;

	/* Disable GPIO */
	HAL_GPIO_DeInit(bsp_i2c_hw[dev_num].scl_port, bsp_i2c_hw[dev_num].scl_pin);
	HAL_GPIO_DeInit(bsp_i2c_hw[dev_num].sda_port, bsp_i2c_hw[dev_num].sda_pin);
}

/** \brief I2C SW Bit Banging GPIO HW Init.
 *
 * \param dev_num bsp_dev_i2c_t: I2C dev num
 * \param gpio_scl_sda_pull uint32_t: MODE_CONFIG_DEV_GPIO_PULLUP/PULLDOWN or NOPULL
 * \return void
 *
 */
static void i2c_gpio_hw_init(bsp_dev_i2c_t dev_num, uint32_t gpio_scl_sda_pull)
{
	GPIO_InitTypeDef gpio_init;

	if(dev_num >= BSP_DEV_I2C_END) return;

	gpio_init.Mode = GPIO_MODE_OUTPUT_OD; /* output open drain */
	gpio_init.Speed = GPIO_SPEED_LOW; /* GPIO Max 8MHz */
	gpio_init.Pull = gpio_scl_sda_pull;
	gpio_init.Alternate = 0; /* Not used */

	if(bsp_i2c_hw[dev_num].scl_port == bsp_i2c_hw[dev_num].sda_port) {
		gpio_init.Pin = bsp_i2c_hw[dev_num].scl_pin | bsp_i2c_hw[dev_num].sda_pin;
		HAL_GPIO_Init(bsp_i2c_hw[dev_num].scl_port, &gpio_init);
	} else {
		gpio_init.Pin = bsp_i2c_hw[dev_num].scl_pin;
		HAL_GPIO_Init(bsp_i2c_hw[dev_num].scl_port, &gpio_init);

		gpio_init.Pin = bsp_i2c_hw[dev_num].sda_pin;
		HAL_GPIO_Init(bsp_i2c_hw[dev_num].sda_port, &gpio_init);
	}
}

/** \brief Init I2C device.
 *
 * \param dev_num bsp_dev_i2c_t: I2C dev num.
 * \param mode_conf mode_config_proto_t*: Mode config proto.
 * \return bsp_status_t: status of the init.
 *
 */
bsp_status_t bsp_i2c_master_init(bsp_dev_i2c_t dev_num, mode_config_proto_t* mode_conf)
{
	uint32_t gpio_scl_sda_pull;

	if(dev_num >= BSP_DEV_I2C_END) return BSP_ERROR;

	bsp_i2c_master_deinit(dev_num);

	/* I2C peripheral configuration */
	if(mode_conf->config.i2c.dev_speed < I2C_SPEED_MAX)
		i2c_speed_delay = SystemCoreClock / (i2c_freqs[mode_conf->config.i2c.dev_speed] * 2);
	else
		return BSP_ERROR;

	i2c_clock_strech_timeout = mode_conf->config.i2c.dev_clock_stretch_timeout;

	/* Init the I2C */
	switch(mode_conf->config.i2c.dev_gpio_pull) {
	case MODE_CONFIG_DEV_GPIO_PULLUP:
		gpio_scl_sda_pull = GPIO_PULLUP;
		break;

	case MODE_CONFIG_DEV_GPIO_PULLDOWN:
		gpio_scl_sda_pull = GPIO_PULLDOWN;
		break;

	default:
	case MODE_CONFIG_DEV_GPIO_NOPULL:
		gpio_scl_sda_pull = GPIO_NOPULL;
		break;
	}
	i2c_gpio_hw_init(dev_num, gpio_scl_sda_pull);

	set_sda_float(dev_num);
	set_scl_float(dev_num);

	i2c_started = FALSE;
	return BSP_OK;
}

/** \brief De-initialize the I2C comunication bus
 *
 * \param dev_num bsp_dev_i2c_t: I2C dev num.
 * \return bsp_status_t: status of the deinit.
 *
 */
bsp_status_t bsp_i2c_master_deinit(bsp_dev_i2c_t dev_num)
{
	if(dev_num >= BSP_DEV_I2C_END) return BSP_ERROR;

	/* DeInit the low level hardware: GPIO, CLOCK, NVIC... */
	i2c_gpio_hw_deinit(dev_num);

	return BSP_OK;
}

/** \brief Sends START BIT in blocking mode and set the status.
 *
 * \param dev_num bsp_dev_i2c_t: I2C dev num.
 * \return bsp_status_t: status of the transfer.
 *
 */
bsp_status_t bsp_i2c_start(bsp_dev_i2c_t dev_num)
{
	if(dev_num >= BSP_DEV_I2C_END) return BSP_ERROR;

	if(i2c_started == TRUE) {
		/* Re-Start condition */
		set_sda_float(dev_num);
		i2c_sw_delay();

		set_scl_float(dev_num);
		i2c_sw_delay();
	}

	/* Generate START */
	/* SDA & SCL are assumed to be floating = HIGH */
	set_sda_low(dev_num);
	i2c_sw_delay();

	set_scl_low(dev_num);

	i2c_started = TRUE;
	return BSP_OK;
}

/** \brief Sends STOP BIT in blocking mode and set the status.
 *
 * \param dev_num bsp_dev_i2c_t: I2C dev num.
 * \return bsp_status_t: status of the transfer.
 *
 */
bsp_status_t bsp_i2c_stop(bsp_dev_i2c_t dev_num)
{
	if(dev_num >= BSP_DEV_I2C_END) return BSP_ERROR;

	/* Generate STOP condition */
	set_sda_low(dev_num);
	i2c_sw_delay();

	set_scl_float(dev_num);
	i2c_sw_delay();

	set_sda_float(dev_num);
	i2c_sw_delay();

	i2c_started = FALSE;
	return BSP_OK;
}

/** \brief Set SCL to float and wait for slave device to be ready
 *
 * \param dev_num bsp_dev_i2c_t: I2C dev num.
 * \return bsp_status_t: status of the transfer.
 *
 */
static bsp_status_t i2c_master_set_scl_float_and_wait_ready(bsp_dev_i2c_t dev_num)
{
	uint32_t clock_stretch_tick_count;
	unsigned char scl_val;

	set_scl_float(dev_num);
	i2c_sw_delay();

	if (i2c_clock_strech_timeout != 0) {
		scl_val = get_scl(dev_num);
		if (scl_val == 0) {
			clock_stretch_tick_count = 0;
			while ((scl_val == 0) && (clock_stretch_tick_count < i2c_clock_strech_timeout)) {
				i2c_sw_delay();
				i2c_sw_delay();

				scl_val = get_scl(dev_num);
				clock_stretch_tick_count++;
			}

			if (clock_stretch_tick_count == i2c_clock_strech_timeout) {
				return BSP_TIMEOUT;
			}
		}
	}

	return BSP_OK;
}

/** \brief Sends a Byte in blocking mode and set the status.
 *
 * \param dev_num bsp_dev_i2c_t: I2C dev num.
 * \param tx_data uint8_t: data to send.
 * \param tx_ack_flag bool*: TRUE means ACK, FALSE means NACK.
 * \return bsp_status_t: status of the transfer.
 *
 */
bsp_status_t bsp_i2c_master_write_u8(bsp_dev_i2c_t dev_num, uint8_t tx_data, uint8_t* tx_ack_flag)
{
	int i;
	unsigned char ack_val;
	bsp_status_t status;

	if(dev_num >= BSP_DEV_I2C_END) return BSP_ERROR;

	/* Write 8 bits */
	for(i = 0; i < 8; i++) {
		if(tx_data & 0x80)
			set_sda_float(dev_num);
		else
			set_sda_low(dev_num);

		i2c_sw_delay();

		status = i2c_master_set_scl_float_and_wait_ready(dev_num);
		if (status != BSP_OK) {
			return status;
		}

		set_scl_low(dev_num);
		tx_data <<= 1;
	}

	/* Read 1 bit ACK or NACK */
	set_sda_float(dev_num);
	i2c_sw_delay();

	status = i2c_master_set_scl_float_and_wait_ready(dev_num);
	if (status != BSP_OK) {
		return status;
	}

	ack_val = get_sda(dev_num);

	set_scl_low(dev_num);
	i2c_sw_delay();

	if(ack_val == 0)
		*tx_ack_flag = TRUE;
	else
		*tx_ack_flag = FALSE;

	return BSP_OK;
}

/** \brief Write ACK or NACK at end of Read.
 *
 * \param dev_num bsp_dev_i2c_t: I2C dev num.
 * \param enable_ack bool: TRUE means ACK, FALSE means NACK.
 * \return void
 *
 */
bsp_status_t bsp_i2c_read_ack(bsp_dev_i2c_t dev_num, bool enable_ack)
{
	bsp_status_t status;

	if(dev_num >= BSP_DEV_I2C_END) return BSP_ERROR;

	/* Write 1 bit ACK or NACK */
	if(enable_ack == TRUE)
		set_sda_low(dev_num); /* ACK */
	else
		set_sda_float(dev_num); /* NACK */

	i2c_sw_delay();

	status = i2c_master_set_scl_float_and_wait_ready(dev_num);
	if (status != BSP_OK) {
		return status;
	}

	set_scl_low(dev_num);

	return BSP_OK;
}

/** \brief Read a Byte in blocking mode and set the status.
 *
 * \param dev_num bsp_dev_i2c_t: I2C dev num.
 * \param rx_data uint8_t*: The received byte.
 * \return bsp_status_t: status of the transfer.
 *
 */
bsp_status_t bsp_i2c_master_read_u8(bsp_dev_i2c_t dev_num, uint8_t* rx_data)
{
	unsigned char data;
	int i;
	bsp_status_t status;

	if(dev_num >= BSP_DEV_I2C_END) return BSP_ERROR;

	/* Read 8 bits */
	data = 0;
	for(i = 0; i < 8; i++) {
		set_sda_float(dev_num);
		i2c_sw_delay();

		status = i2c_master_set_scl_float_and_wait_ready(dev_num);
		if (status != BSP_OK) {
			return status;
		}

		data <<= 1;
		if(get_sda(dev_num))
			data |= 1;

		set_scl_low(dev_num);
		i2c_sw_delay();
	}
	*rx_data = data;

	/* Do not Send ACK / NACK because sent by bsp_i2c_read_ack() */

	return BSP_OK;
}