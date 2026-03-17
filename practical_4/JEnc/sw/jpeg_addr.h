/*
 * jpeg_addr.h — Peripheral base addresses for the JPEG MPSoC design.
 *
 * LED_BASE and OFFSET are used by bmp_extract.c to toggle the onboard
 * LEDs as progress indicators while the BMP file is being read.
 *
 * Replace LED_BASE with the actual address from your system.h if it
 * differs (look for the PIO / LED peripheral definition in system.h).
 */
#ifndef JPEG_ADDR_H
#define JPEG_ADDR_H

#include "system.h"   /* Provides all BSP-generated peripheral addresses */

/*
 * If the BSP defines a LED peripheral, use its base directly.
 * Otherwise fall back to the address from practical_1.
 */
#ifdef LED_PIO_BASE
  #define LED_BASE  LED_PIO_BASE
#elif defined(LED_BASE)
  /* already defined in system.h */
#else
  #define LED_BASE  0x11001000   /* default from practical_1 */
#endif

#define OFFSET  0x00000000

#endif /* JPEG_ADDR_H */
