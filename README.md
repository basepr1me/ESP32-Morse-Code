# ESP32 Morse Code

Arduino ESP32 Morse Code Library

This Morse Code library allows the programmer to generate Morse Code via a GPIO
pin or send audio Morse Code via the on-board DAC.

Usage
-----

Clone the library to your project:

		git clone https://github.com/basepr1me/ESP32-Morse-Code.git libraries/Morse

Two class instantiation methods are in place:

		Morse(TYPE, PIN/DAC_CHANNEL);
		Morse(TYPE, PIN/DAC_CHANNEL, WPM);

Types available are:
		
		M_GPIO	// GPIO Pin Type
		M_DAC	// DAC_CHANNEL Type

It is highly desirable to configure the DAC CW generator by hand:

		dac_cw_config_t dac_cw_config;

		dac_cw_config.en_ch = DAC_CHANNEL_2;
		dac_cw_config.scale = DAC_CW_SCALE_2;
		dac_cw_config.phase = DAC_CW_PHASE_180;
		dac_cw_config.freq = 550;

		morse_dac.dac_cw_configure(&dac_cw_config);

Pro-sign Morse Code can be generated using backticks:

		String tx = "ab2cd de a2bcd `sos`";

To enable the Morse Code, two watchdogs are available:

		morse.gpio_watchdog();
		morse_dac.dac_watchdog();

To set the WPM, two functions are available:

		morse.gpio_set_wpm(WPM)
		morse.dac_set_wpm(WPM)
		

Example
-------

See the [LED_DAC Example](examples/LED_DAC/LED_DAC.ino) file for more
information.

Notes
-----

Only two DAC pins are available on the ESP32. These pins are:

		DAC_CHANNEL_1: 25
		DAC_CHANNEL_2: 26

I don't pretend to know anything about writing Arduino libraries. This style is
OpenBSD style and may not look right in the Arduino IDE, since I don't use it.

Author
------

[Tracey Emery](https://github.com/basepr1me/)

If you like this software, consider [donating](https://k7tle.com/?donate=1).

See the [License](LICENSE.md) file for more information.
