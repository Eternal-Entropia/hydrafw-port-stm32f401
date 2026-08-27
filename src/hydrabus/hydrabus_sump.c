/*
 * HydraBus/HydraNFC
 *
 * Copyright (C) 2014-2015 Benjamin VERNOUX
 * Copyright (C) 2015 Nicolas OBERLI
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "common.h"
#include "tokenline.h"
#include "bsp.h"
#include "bsp_tim.h"
#include "hydrabus_sump.h"
#include "alloc.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define TOTAL_BUFFER_BYTES 32768

static uint8_t g_sump_channels = 16;
static uint32_t INDEX = 0;

static uint32_t get_sump_sample_count(uint8_t ch)
{
	switch(ch) {
	case 1:  return 262144; // 32768 * 8
	case 2:  return 131072; // 32768 * 4
	case 4:  return 65536;  // 32768 * 2
	case 8:  return 32768;  // 32768 * 1
	default: return 16384;  // 16 channels: 32768 / 2
	}
}

static uint32_t get_sump_max_rate(uint8_t ch)
{
	switch(ch) {
	case 1:  return 21000000; // 21 MHz
	case 2:  return 10500000; // 10.5 MHz
	case 4:  return 8000000;  // 8 MHz
	case 8:  return 4000000;  // 4 MHz
	default: return 2000000;  // 2 MHz
	}
}

static const char* get_sump_name(uint8_t ch)
{
	switch(ch) {
	case 1:  return "HydraBus 1ch (256K@21M)";
	case 2:  return "HydraBus 2ch (128K@10.5M)";
	case 4:  return "HydraBus 4ch (64K@8M)";
	case 8:  return "HydraBus 8ch (32K@4M)";
	default: return "HydraBus 16ch (16K@2M)";
	}
}

static uint32_t get_sump_tim_period(uint8_t ch)
{
	switch(ch) {
	case 1:  return 4;
	case 2:  return 8;
	case 4:  return 10;
	case 8:  return 21;
	default: return 21;
	}
}

static void portb_init(uint8_t ch)
{
	GPIO_InitTypeDef gpio_init;
	GPIO_TypeDef *hal_gpio_port;
	hal_gpio_port = (GPIO_TypeDef*)GPIOB;

	uint8_t gpio_pin;
	uint8_t pin_count = (ch > 16) ? 16 : ch;
	if (pin_count < 1) pin_count = 1;

	__GPIOB_CLK_ENABLE();

	gpio_init.Mode = GPIO_MODE_INPUT;
	gpio_init.Speed = GPIO_SPEED_HIGH;
	gpio_init.Pull = GPIO_NOPULL;
	gpio_init.Alternate = 0; /* Not used */

	for(gpio_pin=0; gpio_pin<pin_count; gpio_pin++) {
		HAL_GPIO_DeInit(hal_gpio_port, 1 << gpio_pin);
		gpio_init.Pin = 1 << gpio_pin;
		HAL_GPIO_Init(hal_gpio_port, &gpio_init);
	}
}

static void tim_init(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;
	uint32_t period = get_sump_tim_period(g_sump_channels);
	uint32_t div = proto->config.sump.divider;
	if (div == 0) div = 1;

	if (g_sump_channels == 16) {
		bsp_tim_init(period, 2 * div, TIM_CLOCKDIVISION_DIV1, TIM_COUNTERMODE_UP);
	} else {
		bsp_tim_init(period, div, TIM_CLOCKDIVISION_DIV1, TIM_COUNTERMODE_UP);
	}
}

static void tim_set_prescaler(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;
	uint32_t div = proto->config.sump.divider;
	if (div == 0) div = 1;

	if (g_sump_channels == 16) {
		bsp_tim_set_prescaler(2 * div);
	} else {
		bsp_tim_set_prescaler(div);
	}
}

static void sump_init(t_hydra_console *con)
{
	portb_init(g_sump_channels);
	tim_init(con);
}

static void get_samples(t_hydra_console *con, uint8_t * buffer, uint32_t sample_count) __attribute__((optimize("-O3")));
static void get_samples(t_hydra_console *con, uint8_t * buffer, uint32_t sample_count)
{
	mode_config_proto_t* proto = &con->mode->proto;
	uint32_t config_state;
	config_state = proto->config.sump.state;

	/* Lock Kernel for logic analyzer */
	chSysLock();

	bsp_tim_start();

	uint32_t mask_mod = sample_count - 1;

	if(config_state == SUMP_STATE_ARMED)
	{
		uint32_t config_trigger_value = proto->config.sump.trigger_values[0];
		uint32_t config_trigger_mask = proto->config.sump.trigger_masks[0];

		if (g_sump_channels > 8) {
			uint16_t *buf16 = (uint16_t*)buffer;
			while(1) {
				bsp_tim_wait_irq();
				buf16[INDEX] = (uint16_t)GPIOB->IDR;
				bsp_tim_clr_irq();
				if (!((buf16[INDEX] ^ config_trigger_value) & config_trigger_mask)) {
					config_state = SUMP_STATE_TRIGGED;
					break;
				}
				INDEX = (INDEX + 1) & mask_mod;
			}
		} else if (g_sump_channels == 8) {
			while(1) {
				bsp_tim_wait_irq();
				buffer[INDEX] = (uint8_t)GPIOB->IDR;
				bsp_tim_clr_irq();
				if (!((buffer[INDEX] ^ config_trigger_value) & config_trigger_mask)) {
					config_state = SUMP_STATE_TRIGGED;
					break;
				}
				INDEX = (INDEX + 1) & mask_mod;
			}
		} else if (g_sump_channels == 4) {
			while(1) {
				bsp_tim_wait_irq();
				uint8_t in = (uint8_t)(GPIOB->IDR & 0x0F);
				uint32_t b_idx = INDEX >> 1;
				if (!(INDEX & 1)) {
					buffer[b_idx] = in;
				} else {
					buffer[b_idx] |= (in << 4);
				}
				bsp_tim_clr_irq();
				if (!((in ^ config_trigger_value) & config_trigger_mask)) {
					config_state = SUMP_STATE_TRIGGED;
					break;
				}
				INDEX = (INDEX + 1) & mask_mod;
			}
		} else if (g_sump_channels == 2) {
			while(1) {
				bsp_tim_wait_irq();
				uint8_t in = (uint8_t)(GPIOB->IDR & 0x03);
				uint32_t b_idx = INDEX >> 2;
				uint8_t shift = (INDEX & 3) << 1;
				if (!shift) {
					buffer[b_idx] = in;
				} else {
					buffer[b_idx] |= (in << shift);
				}
				bsp_tim_clr_irq();
				if (!((in ^ config_trigger_value) & config_trigger_mask)) {
					config_state = SUMP_STATE_TRIGGED;
					break;
				}
				INDEX = (INDEX + 1) & mask_mod;
			}
		} else {
			while(1) {
				bsp_tim_wait_irq();
				uint8_t in = (uint8_t)(GPIOB->IDR & 0x01);
				uint32_t b_idx = INDEX >> 3;
				uint8_t shift = (INDEX & 7);
				if (!shift) {
					buffer[b_idx] = in;
				} else {
					buffer[b_idx] |= (in << shift);
				}
				bsp_tim_clr_irq();
				if (!((in ^ config_trigger_value) & config_trigger_mask)) {
					config_state = SUMP_STATE_TRIGGED;
					break;
				}
				INDEX = (INDEX + 1) & mask_mod;
			}
		}
	}

	if(config_state == SUMP_STATE_TRIGGED)
	{
		register uint32_t config_delay_count = proto->config.sump.delay_count;

		if (g_sump_channels > 8) {
			uint16_t *buf16 = (uint16_t*)buffer;
			while(config_delay_count > 0) {
				bsp_tim_wait_irq();
				buf16[INDEX] = (uint16_t)GPIOB->IDR;
				bsp_tim_clr_irq();
				config_delay_count--;
				INDEX = (INDEX + 1) & mask_mod;
			}
		} else if (g_sump_channels == 8) {
			while(config_delay_count > 0) {
				bsp_tim_wait_irq();
				buffer[INDEX] = (uint8_t)GPIOB->IDR;
				bsp_tim_clr_irq();
				config_delay_count--;
				INDEX = (INDEX + 1) & mask_mod;
			}
		} else if (g_sump_channels == 4) {
			while(config_delay_count > 0) {
				bsp_tim_wait_irq();
				uint8_t in = (uint8_t)(GPIOB->IDR & 0x0F);
				uint32_t b_idx = INDEX >> 1;
				if (!(INDEX & 1)) {
					buffer[b_idx] = in;
				} else {
					buffer[b_idx] |= (in << 4);
				}
				bsp_tim_clr_irq();
				config_delay_count--;
				INDEX = (INDEX + 1) & mask_mod;
			}
		} else if (g_sump_channels == 2) {
			while(config_delay_count > 0) {
				bsp_tim_wait_irq();
				uint8_t in = (uint8_t)(GPIOB->IDR & 0x03);
				uint32_t b_idx = INDEX >> 2;
				uint8_t shift = (INDEX & 3) << 1;
				if (!shift) {
					buffer[b_idx] = in;
				} else {
					buffer[b_idx] |= (in << shift);
				}
				bsp_tim_clr_irq();
				config_delay_count--;
				INDEX = (INDEX + 1) & mask_mod;
			}
		} else {
			while(config_delay_count > 0) {
				bsp_tim_wait_irq();
				uint8_t in = (uint8_t)(GPIOB->IDR & 0x01);
				uint32_t b_idx = INDEX >> 3;
				uint8_t shift = (INDEX & 7);
				if (!shift) {
					buffer[b_idx] = in;
				} else {
					buffer[b_idx] |= (in << shift);
				}
				bsp_tim_clr_irq();
				config_delay_count--;
				INDEX = (INDEX + 1) & mask_mod;
			}
		}
	}

	chSysUnlock();
	proto->config.sump.state = SUMP_STATE_IDLE;
	bsp_tim_stop();
}

static void sump_deinit(void)
{
	GPIO_TypeDef *hal_gpio_port;
	hal_gpio_port = (GPIO_TypeDef*)GPIOB;
	uint8_t gpio_pin;

	bsp_tim_deinit();
	for(gpio_pin=0; gpio_pin<16; gpio_pin++) {
		HAL_GPIO_DeInit(hal_gpio_port, 1 << gpio_pin);
	}
}

int cmd_sump(t_hydra_console *con, t_tokenline_parsed *p)
{
	int t;
	if (p) {
		for (t = 1; p->tokens[t] > 0; t++) {
			if (p->tokens[t] == T_PINS) {
				t += 2;
				if (t < TL_MAX_WORDS) {
					uint32_t val = 0;
					memcpy(&val, p->buf + p->tokens[t], sizeof(uint32_t));
					if (val == 1 || val == 2 || val == 4 || val == 8 || val == 16) {
						g_sump_channels = (uint8_t)val;
					}
				}
			} else if (p->tokens[t] == T_ARG_UINT) {
				t += 1;
				if (t < TL_MAX_WORDS) {
					uint32_t val = 0;
					memcpy(&val, p->buf + p->tokens[t], sizeof(uint32_t));
					if (val == 1 || val == 2 || val == 4 || val == 8 || val == 16) {
						g_sump_channels = (uint8_t)val;
					}
				}
			}
		}
	}

	cprintf(con, "SUMP Profile: %s, Buffer: %d samples.\r\n", 
	        get_sump_name(g_sump_channels), get_sump_sample_count(g_sump_channels));
	cprintf(con, "Interrupt by pressing user button.\r\n\r\n");

	sump(con);

	return TRUE;
}

void sump(t_hydra_console *con) __attribute__((optimize("-O3")));
void sump(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;
	uint8_t *buffer = pool_alloc_bytes(TOTAL_BUFFER_BYTES);

	if(buffer == 0) {
		cprintf(con, "Error: memory pool allocation failed.\r\n");
		return;
	}

	sump_init(con);
	proto->config.sump.state = SUMP_STATE_IDLE;

	uint8_t sump_command;
	uint8_t sump_parameters[4] = {0};
	uint32_t index=0;
	uint32_t sample_count = get_sump_sample_count(g_sump_channels);

	while (!hydrabus_ubtn()) {
		if(chnReadTimeout(con->sdu, &sump_command, 1, 1)) {
			switch(sump_command) {
			case SUMP_RESET:
				break;
			case SUMP_ID:
				cprintf(con, "1ALS");
				break;
			case SUMP_RUN:
				INDEX=0;
				proto->config.sump.state = SUMP_STATE_ARMED;
				get_samples(con, buffer, sample_count);

				while(proto->config.sump.read_count > 0) {
					if (INDEX == 0) {
						INDEX = sample_count - 1;
					} else {
						INDEX--;
					}

					if (g_sump_channels > 8) {
						uint16_t v = ((uint16_t*)buffer)[INDEX];
						cprintf(con, "%c%c\x00\x00", v & 0xff, (v >> 8) & 0xff);
					} else if (g_sump_channels == 8) {
						uint8_t v = buffer[INDEX];
						cprintf(con, "%c\x00\x00\x00", v);
					} else if (g_sump_channels == 4) {
						uint8_t byte_val = buffer[INDEX >> 1];
						uint8_t v = (INDEX & 1) ? ((byte_val >> 4) & 0x0F) : (byte_val & 0x0F);
						cprintf(con, "%c\x00\x00\x00", v);
					} else if (g_sump_channels == 2) {
						uint8_t byte_val = buffer[INDEX >> 2];
						uint8_t shift = (INDEX & 3) << 1;
						uint8_t v = (byte_val >> shift) & 0x03;
						cprintf(con, "%c\x00\x00\x00", v);
					} else {
						uint8_t byte_val = buffer[INDEX >> 3];
						uint8_t v = (byte_val >> (INDEX & 7)) & 0x01;
						cprintf(con, "%c\x00\x00\x00", v);
					}
					proto->config.sump.read_count--;
				}
				break;
			case SUMP_DESC:
			{
				uint32_t cur_samples = get_sump_sample_count(g_sump_channels);
				uint32_t cur_rate = get_sump_max_rate(g_sump_channels);
				uint8_t cur_probes = (g_sump_channels > 16) ? 16 : g_sump_channels;
				const char *dev_name = get_sump_name(g_sump_channels);

				// device name string
				cprintf(con, "%c", 0x01);
				cprintf(con, "%s", dev_name);
				cprintf(con, "%c", 0x00);

				// sample memory (32-bit big-endian)
				cprintf(con, "%c", 0x21);
				cprintf(con, "%c", (cur_samples >> 24) & 0xff);
				cprintf(con, "%c", (cur_samples >> 16) & 0xff);
				cprintf(con, "%c", (cur_samples >> 8) & 0xff);
				cprintf(con, "%c", cur_samples & 0xff);

				// sample rate (32-bit big-endian)
				cprintf(con, "%c", 0x23);
				cprintf(con, "%c", (cur_rate >> 24) & 0xff);
				cprintf(con, "%c", (cur_rate >> 16) & 0xff);
				cprintf(con, "%c", (cur_rate >> 8) & 0xff);
				cprintf(con, "%c", cur_rate & 0xff);

				// number of probes
				cprintf(con, "%c", 0x40);
				cprintf(con, "%c", cur_probes);

				// protocol version (2)
				cprintf(con, "%c", 0x41);
				cprintf(con, "%c", 0x02);
				cprintf(con, "%c", 0x00);
				break;
			}
			case SUMP_XON:
			case SUMP_XOFF:
				/* not implemented */
				break;
			default:
				// Other commands take 4 bytes as parameters
				if(chnRead(con->sdu, sump_parameters, 4) == 4) {
					switch(sump_command) {
					case SUMP_TRIG_1:
					case SUMP_TRIG_2:
					case SUMP_TRIG_3:
					case SUMP_TRIG_4:
						// Get the trigger index
						index = (sump_command & 0x0c) >> 2;
						proto->config.sump.trigger_masks[index] = sump_parameters[3];
						proto->config.sump.trigger_masks[index] <<= 8;
						proto->config.sump.trigger_masks[index] |= sump_parameters[2];
						proto->config.sump.trigger_masks[index] <<= 8;
						proto->config.sump.trigger_masks[index] |= sump_parameters[1];
						proto->config.sump.trigger_masks[index] <<= 8;
						proto->config.sump.trigger_masks[index] |= sump_parameters[0];
						break;
					case SUMP_TRIG_VALS_1:
					case SUMP_TRIG_VALS_2:
					case SUMP_TRIG_VALS_3:
					case SUMP_TRIG_VALS_4:
						// Get the trigger index
						index = (sump_command & 0x0c) >> 2;
						proto->config.sump.trigger_values[index] = sump_parameters[3];
						proto->config.sump.trigger_values[index] <<= 8;
						proto->config.sump.trigger_values[index] |= sump_parameters[2];
						proto->config.sump.trigger_values[index] <<= 8;
						proto->config.sump.trigger_values[index] |= sump_parameters[1];
						proto->config.sump.trigger_values[index] <<= 8;
						proto->config.sump.trigger_values[index] |= sump_parameters[0];
						break;
					case SUMP_CNT:
						proto->config.sump.delay_count = sump_parameters[3];
						proto->config.sump.delay_count <<= 8;
						proto->config.sump.delay_count |= sump_parameters[2];
						proto->config.sump.delay_count <<= 2; /* values are multiples of 4 */
						proto->config.sump.read_count = sump_parameters[1];
						proto->config.sump.read_count <<= 8;
						proto->config.sump.read_count |= sump_parameters[0];
						proto->config.sump.read_count++;
						proto->config.sump.read_count <<= 2; /* values are multiples of 4 */
						break;
					case SUMP_DIV:
						proto->config.sump.divider = sump_parameters[2];
						proto->config.sump.divider <<= 8;
						proto->config.sump.divider |= sump_parameters[1];
						proto->config.sump.divider <<= 8;
						proto->config.sump.divider |= sump_parameters[0];
						proto->config.sump.divider /= 50; /* Assuming 100MHz base frequency */
						proto->config.sump.divider++;
						tim_set_prescaler(con);
						break;
					case SUMP_FLAGS:
						proto->config.sump.channels = (~sump_parameters[0] >> 2) & 0x0f;
						/* not implemented */
						break;
					default:
						break;
					}
				}
				break;
			}
		}
	}
	pool_free(buffer);
	sump_deinit();
}


