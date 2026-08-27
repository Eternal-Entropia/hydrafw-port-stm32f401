/*
HydraBus/HydraNFC - Copyright (C) 2014 Benjamin VERNOUX

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

#ifndef _BSP_I2C_CONF_H_
#define _BSP_I2C_CONF_H_

/* I2C1: SCL=PB6, SDA=PB7 */
#define BSP_I2C1_SCL_PORT           GPIOB
#define BSP_I2C1_SCL_PIN            GPIO_PIN_6
#define BSP_I2C1_SDA_PORT           GPIOB
#define BSP_I2C1_SDA_PIN            GPIO_PIN_7

/* I2C2: SCL=PB10, SDA=PB3 */
#define BSP_I2C2_SCL_PORT           GPIOB
#define BSP_I2C2_SCL_PIN            GPIO_PIN_10
#define BSP_I2C2_SDA_PORT           GPIOB
#define BSP_I2C2_SDA_PIN            GPIO_PIN_3

/* I2C3: SCL=PA8, SDA=PB4 */
#define BSP_I2C3_SCL_PORT           GPIOA
#define BSP_I2C3_SCL_PIN            GPIO_PIN_8
#define BSP_I2C3_SDA_PORT           GPIOB
#define BSP_I2C3_SDA_PIN            GPIO_PIN_4

#endif /* _BSP_I2C_CONF_H_ */
