/*
 * HydraBus/HydraNFC
 *
 * Copyright (C) 2014-2015 Benjamin VERNOUX
 * Copyright (C) 2016 Nicolas OBERLI
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
#include "hydrabus.h"
#include "bsp.h"
#include "bsp_freq.h"

#include <string.h>

int cmd_freq(t_hydra_console *con, t_tokenline_parsed *p)
{
	uint32_t frequency, duty;
	mode_config_proto_t* proto = &con->mode->proto;
	(void) p;

	bsp_freq_get_values(proto->dev_num, &frequency, &duty);
	cprintf(con, "Frequency : %dHz\r\n", frequency);
	cprintf(con, "Duty : %d%%\r\n", duty);
	cprintf(con, "\r\n");

	bsp_freq_deinit(proto->dev_num);

	return TRUE;
}

