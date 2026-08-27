/*
HydraBus/HydraNFC - Copyright (C) 2014 Benjamin VERNOUX
HydraBus/HydraNFC - Copyright (C) 2016 Nicolas OBERLI

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

#ifndef _BSP_FREQ_CONF_H_
#define _BSP_FREQ_CONF_H_

/* FREQ1 -> PB4 (TIM3_CH1 on BlackPill) */
#define BSP_FREQ1_TIMER TIM3
#define BSP_FREQ1_AF	GPIO_AF2_TIM3
#define BSP_FREQ1_CLK_ENABLE __TIM3_CLK_ENABLE
#define BSP_FREQ1_PORT	GPIOB
#define BSP_FREQ1_PIN	GPIO_PIN_4 // PB.04
#define BSP_FREQ1_CHAN	TIM_CHANNEL_1

#endif /* _BSP_FREQ_CONF_H_ */
