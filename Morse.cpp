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

#define D_WPM		 15

#define MAX_RAD		 127.5

#define UNIT_T(x)	 (60.0 / (50.0 * (float)x)) * 1000.0

volatile uint8_t	 gpio_wpm, dac_wpm;

volatile uint8_t	 gpio_tx_pin, dac_tx_pin;

volatile float		 gpio_unit_t, dac_unit_t;

volatile uint8_t	 gpio_tx_sending = 0, dac_tx_sending = 0;
uint8_t			 gpio_tx_len, gpio_tx_enc, dac_tx_len, dac_tx_enc;

String			 gpio_tx_str, dac_tx_str;

uint8_t			 gpio_this_index = 0, gpio_next_index = 0;
uint8_t			 gpio_handle_unit, gpio_unit_handled, gpio_bit;
uint8_t			 gpio_digraph = 0, gpio_inited = 0;

uint8_t			 dac_this_index = 0, dac_next_index = 0;
uint8_t			 dac_handle_unit, dac_unit_handled, dac_bit;
uint8_t			 dac_digraph = 0, dac_inited = 0, dac_on = 0;

unsigned long		 gpio_tx_start_millis, gpio_tx_current_millis;
unsigned long		 gpio_handle_unit_millis;

unsigned long		 dac_tx_start_millis, dac_tx_current_millis;
unsigned long		 dac_handle_unit_millis;

uint8_t			 ctob(uint8_t c);

dac_channel_t		 dac_channel;

void			 gpio_stop(void);
void			 gpio_handle_chars(void);
void			 gpio_handle_units(uint8_t c);

void			 dac_stop(void);
void			 dac_handle_chars(void);
void			 dac_handle_units(uint8_t c);

void			 dac_cw_start(void);
void			 dac_cw_stop(void);

Morse::Morse(uint8_t type, uint8_t the_pin)
{
	Morse(type, the_pin, D_WPM);
}

Morse::Morse(uint8_t type, uint8_t the_pin, uint8_t the_wpm)
{
	switch(type) {
	case M_GPIO:
		if (!gpio_inited) {
			gpio_wpm = the_wpm;
			gpio_tx_pin = the_pin;
			gpio_unit_t = UNIT_T(gpio_wpm);
			gpio_inited = 1;
		}
		break;
	case M_DAC:
		if (!dac_inited) {
			dac_wpm = the_wpm;
			dac_unit_t = UNIT_T(dac_wpm);
			dac_cw_config_t dac_cw_config;
			// this func is broken and only gives 1/2 wave form
			/* dac_cw_config.en_ch = channel; */
			dac_cw_config.freq = 1000;

			switch (the_pin) {
			case 1:
				dac_channel = DAC_CHANNEL_1;
				SET_PERI_REG_BITS(SENS_SAR_DAC_CTRL2_REG,
				    SENS_DAC_INV1, 2, SENS_DAC_INV1_S);
				break;
			case 2:
				dac_channel = DAC_CHANNEL_2;
				SET_PERI_REG_BITS(SENS_SAR_DAC_CTRL2_REG,
				    SENS_DAC_INV2, 2, SENS_DAC_INV2_S);
				break;
			default:
				break;
			}

			dac_cw_generator_config(&dac_cw_config);
			dac_output_enable(dac_channel);
			dac_inited = 1;
		}
		break;
	default:
		exit(1);
		break;
	}
}

void
Morse::dac_cw_setup(dac_cw_config_t *dac_cw_config)
{
	switch (dac_channel) {
	case 1:
		dac_channel = DAC_CHANNEL_1;
		SET_PERI_REG_BITS(SENS_SAR_DAC_CTRL2_REG,
		    SENS_DAC_INV1, 2, SENS_DAC_INV1_S);
		break;
	case 2:
		dac_channel = DAC_CHANNEL_2;
		SET_PERI_REG_BITS(SENS_SAR_DAC_CTRL2_REG,
		    SENS_DAC_INV2, 2, SENS_DAC_INV2_S);
		break;
	}

	dac_cw_generator_config(dac_cw_config);
	dac_output_enable(dac_channel);
}
// gpio functions

uint8_t
Morse::gpio_transmitting(void)
{
	if (gpio_tx_sending)
		return 1;

	return 0;
}

void
Morse::gpio_tx_stop(void)
{
	gpio_stop();
}

void
Morse::gpio_watchdog(void)
{
	if (gpio_inited)
		gpio_handle_chars();
}

void
Morse::gpio_tx(String tx)
{
	// if we are sending, return
	if (gpio_tx_sending)
		return;

	// start by setting our tx pin low
	digitalWrite(gpio_tx_pin, LOW);

	// gather our starting variables
	gpio_tx_sending = 1;
	gpio_tx_len = tx.length();
	tx.toUpperCase();
	gpio_tx_str = tx;

	gpio_this_index = 0;
	gpio_next_index = 1;

	gpio_tx_start_millis = millis();
	gpio_tx_current_millis = millis();
	gpio_handle_unit_millis = millis();
}

void
Morse::gpio_set_wpm(uint8_t the_wpm)
{
	gpio_wpm = the_wpm;
	gpio_unit_t = UNIT_T(gpio_wpm);
	gpio_inited = 1;
}

void
gpio_handle_chars(void)
{
	gpio_tx_current_millis = millis();
	if (gpio_next_index) {
		if (gpio_this_index == gpio_tx_len) {
			gpio_stop();
			return;
		}
		gpio_tx_start_millis = gpio_tx_current_millis;
		gpio_next_index = 0;
		gpio_handle_unit = 0;
		gpio_unit_handled = 0;
		if (gpio_tx_str[gpio_this_index] == 126) {
			gpio_this_index++;
			gpio_next_index = 1;
		} else if (gpio_tx_str[gpio_this_index] == 96) {
			gpio_digraph = !gpio_digraph;
			gpio_this_index++;
			gpio_next_index = 1;
		} else {
			gpio_tx_enc = ctob(gpio_tx_str[gpio_this_index]);
			gpio_handle_units(gpio_tx_enc);
			gpio_bit = 0;
		}
	} else if (gpio_tx_sending)
		gpio_handle_units(gpio_tx_enc);
}

void
gpio_handle_units(uint8_t c)
{
	// set led on and space off
	if (!gpio_next_index && !gpio_handle_unit && !gpio_unit_handled &&
	    gpio_tx_sending) {
		if (c == 1) {
			gpio_handle_unit_millis = W_SP * gpio_unit_t +
			    (gpio_tx_current_millis - gpio_tx_start_millis);
		} else {
			if (bitRead(c, gpio_bit)) {
				gpio_handle_unit_millis = DAH * gpio_unit_t +
				    (gpio_tx_current_millis -
				     gpio_tx_start_millis);
				digitalWrite(gpio_tx_pin, HIGH);
			} else {
				gpio_handle_unit_millis = DIT * gpio_unit_t +
				    (gpio_tx_current_millis -
				     gpio_tx_start_millis);
				digitalWrite(gpio_tx_pin, HIGH);
			}
		}
		gpio_handle_unit = 1;
		gpio_tx_start_millis = gpio_tx_current_millis;
	}

	// set led off, handle IC_SP, or handle C_SP
	if (gpio_handle_unit && !gpio_unit_handled && !gpio_next_index &&
	    gpio_tx_sending &&
	    (millis() - gpio_tx_start_millis) >= gpio_handle_unit_millis) {
		gpio_tx_start_millis = gpio_tx_current_millis;
		digitalWrite(gpio_tx_pin, LOW);
		gpio_bit++;

		if (c >> (gpio_bit + 1) || gpio_digraph)
			gpio_handle_unit_millis = IC_SP * gpio_unit_t +
			    (gpio_tx_current_millis - gpio_tx_start_millis);
		else
			gpio_handle_unit_millis = C_SP * gpio_unit_t +
			    (gpio_tx_current_millis - gpio_tx_start_millis);

		gpio_unit_handled = 1;
	}

	// we're done, start next
	if (gpio_unit_handled && gpio_tx_sending &&
	    (millis() - gpio_tx_start_millis) >= gpio_handle_unit_millis) {
		gpio_tx_start_millis = gpio_tx_current_millis;
		gpio_unit_handled = 0;
		gpio_handle_unit = 0;

		// hit the end of that byte
		if (!(c >> (gpio_bit + 1))) {
			gpio_bit = 0;
			gpio_next_index = 1;
			gpio_this_index++;
		}

		// hit the end of the tx
		if (gpio_this_index == gpio_tx_len) {
			gpio_stop();
			return;
		}
	}
}

void
gpio_stop(void)
{
	gpio_digraph = 0;
	gpio_tx_sending = 0;
	gpio_this_index = 0;
	gpio_next_index = 0;
}

// dac functions

uint8_t
Morse::dac_transmitting(void)
{
	if (dac_tx_sending)
		return 1;

	return 0;
}

void
Morse::dac_tx_stop(void)
{
	dac_stop();
}

void
Morse::dac_watchdog(void)
{
	if (dac_inited) {
		dac_handle_chars();
		if (dac_on)
			dac_cw_start();
		else
			dac_cw_stop();
	}
}

void
Morse::dac_tx(String tx)
{
	// if we are sending, return
	if (dac_tx_sending)
		return;

	// start by setting our tx pin low
	switch(dac_tx_pin) {
	case 1:
		digitalWrite(25, LOW);
		break;
	case 2:
		digitalWrite(26, LOW);
		break;
	default:
		break;
	}

	// gather our starting variables
	dac_tx_sending = 1;
	dac_tx_len = tx.length();
	tx.toUpperCase();
	dac_tx_str = tx;

	dac_this_index = 0;
	dac_next_index = 1;

	dac_tx_start_millis = millis();
	dac_tx_current_millis = millis();
	dac_handle_unit_millis = millis();
}

void
Morse::dac_cw_configure(dac_cw_config_t *dac_cw_config)
{
	dac_cw_generator_config(dac_cw_config);
}

void
Morse::dac_set_wpm(uint8_t the_wpm)
{
	dac_wpm = the_wpm;
	dac_unit_t = UNIT_T(gpio_wpm);
	dac_inited = 1;
}

void
dac_handle_chars(void)
{
	dac_tx_current_millis = millis();
	if (dac_next_index) {
		if (dac_this_index == dac_tx_len) {
			dac_stop();
			return;
		}
		dac_tx_start_millis = dac_tx_current_millis;
		dac_next_index = 0;
		dac_handle_unit = 0;
		dac_unit_handled = 0;
		if (dac_tx_str[dac_this_index] == 126) {
			dac_this_index++;
			dac_next_index = 1;
		} else if (dac_tx_str[dac_this_index] == 96) {
			dac_digraph = !dac_digraph;
			dac_this_index++;
			dac_next_index = 1;
		} else {
			dac_tx_enc = ctob(dac_tx_str[dac_this_index]);
			dac_handle_units(dac_tx_enc);
			dac_bit = 0;
		}
	} else if (dac_tx_sending)
		dac_handle_units(dac_tx_enc);
}

void
dac_handle_units(uint8_t c)
{
	// set led on and space off
	if (!dac_next_index && !dac_handle_unit && !dac_unit_handled &&
	    dac_tx_sending) {
		if (c == 1) {
			dac_handle_unit_millis = W_SP * dac_unit_t +
			    (dac_tx_current_millis - dac_tx_start_millis);
		} else {
			if (bitRead(c, dac_bit)) {
				dac_handle_unit_millis = DAH * dac_unit_t +
				    (dac_tx_current_millis -
				     dac_tx_start_millis);
				digitalWrite(dac_tx_pin, HIGH);
			} else {
				dac_handle_unit_millis = DIT * dac_unit_t +
				    (dac_tx_current_millis -
				     dac_tx_start_millis);
				digitalWrite(dac_tx_pin, HIGH);
			}
			dac_on = 1;
		}
		dac_handle_unit = 1;
		dac_tx_start_millis = dac_tx_current_millis;
	}

	// set dac off, handle IC_SP, or handle C_SP
	if (dac_handle_unit && !dac_unit_handled && !dac_next_index &&
	    dac_tx_sending &&
	    (millis() - dac_tx_start_millis) >= dac_handle_unit_millis) {
		dac_tx_start_millis = dac_tx_current_millis;
		digitalWrite(dac_tx_pin, LOW);
		dac_bit++;

		if (c >> (dac_bit + 1) || dac_digraph)
			dac_handle_unit_millis = IC_SP * dac_unit_t +
			    (dac_tx_current_millis - dac_tx_start_millis);
		else
			dac_handle_unit_millis = C_SP * dac_unit_t +
			    (dac_tx_current_millis - dac_tx_start_millis);

		dac_on = 0;
		dac_unit_handled = 1;
	}

	// we're done, start next
	if (dac_unit_handled && dac_tx_sending &&
	    (millis() - dac_tx_start_millis) >= dac_handle_unit_millis) {
		dac_tx_start_millis = dac_tx_current_millis;
		dac_unit_handled = 0;
		dac_handle_unit = 0;

		// hit the end of that byte
		if (!(c >> (dac_bit + 1))) {
			dac_bit = 0;
			dac_next_index = 1;
			dac_this_index++;
		}

		// hit the end of the tx
		if (dac_this_index == dac_tx_len) {
			dac_stop();
			return;
		}
	}
}

void
dac_stop(void)
{
	dac_digraph = 0;
	dac_tx_sending = 0;
	dac_this_index = 0;
	dac_next_index = 0;
}

void
dac_cw_start(void)
{
	dac_cw_generator_enable();
}

void
dac_cw_stop(void)
{
	dac_cw_generator_disable();
}

uint8_t
ctob(uint8_t c)
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
	case 61:	return 0b110001;		// '=' 'BT'
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
