#include "Morse.h"

#define	WPM		 17
#define LEDP		 2
#define GSECS		 2000 // seconds between transmittions
#define DSECS		 4000

unsigned long	 current_gpio_millis, start_gpio_millis;
unsigned long	 current_dac_millis, start_dac_millis;
void		 setup(void);
void		 loop(void);


Morse		 morse(M_GPIO, LEDP); // WPM is auto selected
Morse		 morse_dac(M_DAC, DAC_CHANNEL_2, WPM); // use DAC channel 2

// backticks will toggle digraph sending

String		 tx = "ab7cd de a7bcd k `ar`";

void
setup(void)
{
	Serial.begin(115200);

	pinMode(LEDP, OUTPUT);
	digitalWrite(LEDP, LOW);

	// configure our DAC if we want to (this is the prefered method)
	dac_cw_config_t dac_cw_config;

	dac_cw_config.en_ch = DAC_CHANNEL_2;
	dac_cw_config.scale = DAC_CW_SCALE_2;
	dac_cw_config.phase = DAC_CW_PHASE_180;
	dac_cw_config.freq = 550;

	morse_dac.dac_cw_configure(&dac_cw_config);

	// setup millis
	start_gpio_millis = millis();
	current_gpio_millis = millis();

	start_dac_millis = millis();
	current_dac_millis = millis();

	delay(2000);
	Serial.printf("LED and DAC %d setup\r\n", DAC_CHANNEL);
}

void
loop(void)
{
	current_gpio_millis = millis();
	current_dac_millis = millis();

	morse.gpio_watchdog();
	morse_dac.dac_watchdog();

	// it's up to the programmer to properly space transmissions
	if (!morse.gpio_transmitting() &&
	    (current_gpio_millis - start_gpio_millis >= GSECS)) {
		Serial.println("Transmit via LED");
		morse.gpio_tx(tx);
	} else if (morse.gpio_transmitting())
		start_gpio_millis = current_gpio_millis;

	if (!morse.dac_transmitting() &&
	    (current_dac_millis - start_dac_millis >= DSECS)) {
		Serial.println("Transmit via DAC");
		morse_dac.dac_tx(tx);
	} else if (morse.dac_transmitting())
		start_dac_millis = current_dac_millis;
}
