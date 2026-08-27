/*
HydraBus/HydraNFC - Copyright (C) 2014-2015 Benjamin VERNOUX

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

#ifndef _BSP_SPI_CONF_H_
#define _BSP_SPI_CONF_H_

/* SPI1 (External pins PA15, PB3-PB5 mapped to hardware SPI3) */
#define BSP_SPI1              SPI3
#define BSP_SPI1_GPIO_SPEED   GPIO_SPEED_HIGH
#define BSP_SPI1_AF           GPIO_AF6_SPI3
/* SPI1 NSS */
#define BSP_SPI1_NSS_PORT     GPIOA
#define BSP_SPI1_NSS_PIN      GPIO_PIN_15  /* PA.15 SW NSS */
/* SPI1 SCK */
#define BSP_SPI1_SCK_PORT     GPIOB
#define BSP_SPI1_SCK_PIN      GPIO_PIN_3  /* PB.03 */
/* SPI1 MISO */
#define BSP_SPI1_MISO_PORT    GPIOB
#define BSP_SPI1_MISO_PIN     GPIO_PIN_4  /* PB.04 */
/* SPI1 MOSI */
#define BSP_SPI1_MOSI_PORT    GPIOB
#define BSP_SPI1_MOSI_PIN     GPIO_PIN_5  /* PB.05 */

/* SPI2 (BlackPill F401: PC1-PC3 missing in LQFP48, remapped to PB12-PB15) */
#define BSP_SPI2              SPI2
#define BSP_SPI2_GPIO_SPEED   GPIO_SPEED_HIGH
#define BSP_SPI2_AF           GPIO_AF5_SPI2
/* SPI2 NSS */
#define BSP_SPI2_NSS_PORT     GPIOB
#define BSP_SPI2_NSS_PIN      GPIO_PIN_12 /* PB.12 SW NSS */
/* SPI2 SCK */
#define BSP_SPI2_SCK_PORT     GPIOB
#define BSP_SPI2_SCK_PIN      GPIO_PIN_13 /* PB.13 */
/* SPI2 MISO */
#define BSP_SPI2_MISO_PORT    GPIOB
#define BSP_SPI2_MISO_PIN     GPIO_PIN_14 /* PB.14 */
/* SPI2 MOSI */
#define BSP_SPI2_MOSI_PORT    GPIOB
#define BSP_SPI2_MOSI_PIN     GPIO_PIN_15 /* PB.15 */

/* SPI3 (BlackPill bottom SOIC-8 footprint PA4-PA7 mapped to hardware SPI1) */
#define BSP_SPI3              SPI1
#define BSP_SPI3_GPIO_SPEED   GPIO_SPEED_HIGH
#define BSP_SPI3_AF           GPIO_AF5_SPI1
/* SPI3 NSS */
#define BSP_SPI3_NSS_PORT     GPIOA
#define BSP_SPI3_NSS_PIN      GPIO_PIN_4 /* PA.04 SW NSS */
/* SPI3 SCK */
#define BSP_SPI3_SCK_PORT     GPIOA
#define BSP_SPI3_SCK_PIN      GPIO_PIN_5 /* PA.05 */
/* SPI3 MISO */
#define BSP_SPI3_MISO_PORT    GPIOA
#define BSP_SPI3_MISO_PIN     GPIO_PIN_6 /* PA.06 */
/* SPI3 MOSI */
#define BSP_SPI3_MOSI_PORT    GPIOA
#define BSP_SPI3_MOSI_PIN     GPIO_PIN_7 /* PA.07 */

#endif /* _BSP_SPI_CONF_H_ */

