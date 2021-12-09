#include "Morse/Morse.h"

#define	WPM	 17
#define LEDP	 2
#define SECS	 2000 // seconds between transmittions

unsigned long	 current_millis, start_millis;
void		 setup(void);
void		 loop(void);

Morse		 morse(WPM, LEDP);

// backticks will toggle digraph sending

String		 tx = "k7su de k7tle k `ar`";

void
setup(void)
{
	Serial.begin(115200);

	pinMode(LEDP, OUTPUT);
	digitalWrite(LEDP, LOW);

	start_millis = millis();
	current_millis = millis();

	// send first without 2 second pause
	delay(1000);
	morse.tx_gpio(tx);
}

void
loop(void)
{
	current_millis = millis();
	morse.watchdog();

	// it's up to the programmer to properly space transmissions
	if (!morse.transmitting() &&
	    (current_millis - start_millis >= SECS)) {
		morse.tx_gpio(tx);
	} else if (morse.transmitting())
		start_millis = current_millis;
}
