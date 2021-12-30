/*
 * Copyright (c) 2021, 2022 Tracey Emery K7TLE
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _MORSE_H
#define _MORSE_H

#include "Arduino.h"

class
Morse
{
	// we've no rx code yet
	public:
		Morse(uint8_t type, uint8_t the_pin);
		Morse(uint8_t type, uint8_t the_pin, uint8_t the_wpm);

		uint8_t gpio_transmitting(void);
		void gpio_tx(String tx);
		void gpio_tx_stop(void);
		void gpio_watchdog(void);

		uint8_t dac_transmitting(void);
		void dac_tx(String tx);
		void dac_tx_stop(void);
		void dac_watchdog(void);

	// private:
};

enum
{
	M_GPIO,
	M_DAC,
	M_ADC
};

#endif // _MORSE_H
