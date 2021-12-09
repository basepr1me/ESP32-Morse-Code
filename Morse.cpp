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


#include "Morse.h"

#define DIT		 1.0
#define DAH		 3.0
#define IC_SP		 1.0
#define C_SP		 3.0
#define W_SP		 7.0

volatile uint8_t	 wpm;
volatile uint8_t	 tx_pin;
volatile uint8_t	 rx_pin;
volatile float		 unit_t;

// tx
volatile uint8_t	 tx_gpio_send = 0;
uint8_t			 tx_len, tx_enc;
String			 tx_str;

uint8_t			 this_index = 0, next_index = 0, digraph = 0;
uint8_t			 handle_unit, unit_handled, bit;

unsigned long		 gpio_tx_start_millis, gpio_tx_current_millis;
unsigned long		 handle_unit_millis;

uint8_t			 ctob(uint8_t c);
void			 stop(void);
void			 handle_chars(void);
void			 handle_units(uint8_t c);

Morse::Morse(uint8_t the_wpm, uint8_t the_tx_pin)
{
	init(the_wpm, the_tx_pin, 0);
}

Morse::Morse(uint8_t the_wpm, uint8_t the_tx_pin, uint8_t the_rx_pin)
{
	init(the_wpm, the_tx_pin, the_rx_pin);

}

uint8_t Morse::transmitting(void)
{
	if (tx_gpio_send)
		return 1;

	return 0;
}

void Morse::init(uint8_t the_wpm, uint8_t the_tx_pin, uint8_t the_rx_pin)
{
	wpm = the_wpm;
	tx_pin = the_tx_pin;
	rx_pin = the_rx_pin;

	// calculate our unit time
	unit_t = (60.0 / (50.0 * (float)wpm)) * 1000.0;
}

void Morse::tx_stop(void)
{
	stop();
}

void Morse::watchdog(void)
{
	handle_chars();
}

void Morse::tx_gpio(String tx)
{
	// if we are sending, this is our loop runner for handling chars
	if (tx_gpio_send)
		return;

	// start by setting our tx pin low
	digitalWrite(tx_pin, LOW);

	// gather our starting variables
	tx_gpio_send = 1;
	tx_len = tx.length();
	tx.toUpperCase();
	tx_str = tx;

	this_index = 0;
	next_index = 1;

	gpio_tx_start_millis = millis();
	gpio_tx_current_millis = millis();
	handle_unit_millis = millis();
}

void handle_chars(void)
{
	gpio_tx_current_millis = millis();
	if (next_index) {
		if (this_index == tx_len) {
			stop();
			return;
		}
		gpio_tx_start_millis = gpio_tx_current_millis;
		next_index = 0;
		handle_unit = 0;
		unit_handled = 0;
		if (tx_str[this_index] == 126) {
			this_index++;
			next_index = 1;
		} else if (tx_str[this_index] == 96) {
			digraph = !digraph;
			this_index++;
			next_index = 1;
		} else {
			tx_enc = ctob(tx_str[this_index]);
			handle_units(tx_enc);
			bit = 0;
		}
	} else if (tx_gpio_send)
		handle_units(tx_enc);
}

void handle_units(uint8_t c)
{
	// set led on and space off
	if (!next_index && !handle_unit && !unit_handled && tx_gpio_send) {
		if (c == 1) {
			handle_unit_millis = W_SP * unit_t +
			    (gpio_tx_current_millis - gpio_tx_start_millis);
		} else {
			if (bitRead(c, bit)) {
				handle_unit_millis = DAH * unit_t +
				    (gpio_tx_current_millis -
				     gpio_tx_start_millis);
				digitalWrite(tx_pin, HIGH);
			} else {
				handle_unit_millis = DIT * unit_t +
				    (gpio_tx_current_millis -
				     gpio_tx_start_millis);
				digitalWrite(tx_pin, HIGH);
			}
		}
		handle_unit = 1;
		gpio_tx_start_millis = gpio_tx_current_millis;
	}

	// set led off, handle IC_SP, or handle C_SP
	if (handle_unit && !unit_handled && !next_index && tx_gpio_send &&
	    (millis() - gpio_tx_start_millis) >= handle_unit_millis) {
		gpio_tx_start_millis = gpio_tx_current_millis;
		digitalWrite(tx_pin, LOW);
		bit++;

		if (c >> (bit + 1) || digraph)
			handle_unit_millis = IC_SP * unit_t +
			    (gpio_tx_current_millis - gpio_tx_start_millis);
		else
			handle_unit_millis = C_SP * unit_t +
			    (gpio_tx_current_millis - gpio_tx_start_millis);

		unit_handled = 1;
	}

	// we're done, start next
	if (unit_handled && tx_gpio_send &&
	    (millis() - gpio_tx_start_millis) >= handle_unit_millis) {
		gpio_tx_start_millis = gpio_tx_current_millis;
		unit_handled = 0;
		handle_unit = 0;

		// hit the end of that byte
		if (!(c >> (bit + 1))) {
			bit = 0;
			next_index = 1;
			this_index++;
		}

		// hit the end of the tx
		if (this_index == tx_len) {
			stop();
			return;
		}
	}
}

void stop(void)
{
	digraph = 0;
	tx_gpio_send = 0;
	this_index = 0;
	next_index = 0;
}

uint8_t ctob(uint8_t c)
{
	switch(c) {
	case 32:	return 0b1;			// ' '
	case 33:	return 0b1110101;		// '!'
	case 34:	return 0b1010010;		// '"'
	case 36:	return 0b11001000;		// '$'
	case 38:	return 0b100010;		// '&'
	case 39:	return 0b1011110;		// '\''
	case 40:	return 0b101101;		// '('
	case 41:	return 0b1101101;		// ')'
	case 43:	return 0b101010;		// '+' 'AR'
	case 44:	return 0b1110011;		// ','
	case 45:	return 0b1100001;		// '-'
	case 46:	return 0b1101010;		// '.'
	case 47:	return 0b101001;		// '/'

	case 48:	return 0b111111;		// '0'
	case 49:	return 0b111110;		// '1'
	case 50:	return 0b111100;		// '2'
	case 51:	return 0b111000;		// '3'
	case 52:	return 0b110000;		// '4'
	case 53:	return 0b100000;		// '5'
	case 54:	return 0b100001;		// '6'
	case 55:	return 0b100011;		// '7'
	case 56:	return 0b100111;		// '8'
	case 57:	return 0b101111;		// '9'

	case 58:	return 0b1000111;		// ':'
	case 59:	return 0b1010101;		// ';'
	case 61:	return 0b110001;		// '=' ' BT'
	case 63:	return 0b1001100;		// '?'
	case 64:	return 0b1010110;		// '@'

	case 65:	return 0b110;			// 'A'
	case 66:	return 0b10001;			// 'B'
	case 67:	return 0b10101;			// 'C'
	case 68:	return 0b1001;			// 'D'
	case 69:	return 0b10;			// 'E'
	case 70:	return 0b10100;			// 'F'
	case 71:	return 0b1011;			// 'G'
	case 72:	return 0b10000;			// 'H'
	case 73:	return 0b100;			// 'I'
	case 74:	return 0b11110;			// 'J'
	case 75:	return 0b1101;			// 'K'
	case 76:	return 0b10010;			// 'L'
	case 77:	return 0b111;			// 'M'
	case 78:	return 0b101;			// 'N'
	case 79:	return 0b1111;			// 'O'
	case 80:	return 0b10110;			// 'P'
	case 81:	return 0b11011;			// 'Q'
	case 82:	return 0b1010;			// 'R'
	case 83:	return 0b1000;			// 'S'
	case 84:	return 0b11;			// 'T'
	case 85:	return 0b1100;			// 'U'
	case 86:	return 0b11000;			// 'V'
	case 87:	return 0b1110;			// 'W'
	case 88:	return 0b11001;			// 'X'
	case 89:	return 0b11101;			// 'Y'
	case 90:	return 0b10011;			// 'Z'

	case 95:	return 0b1101100;		// '_'

	default:	return 0b11000000;		// non (126)
	}
};
