/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
     along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "esp_system.h"
#include "driver/gpio.h"
#include "sys/time.h"

int TRIGGER;
int ECHO;
int hc_init = 0;
struct timeval tv = { .tv_sec = 0, .tv_usec = 0 };

// Returns temperature from sensor
float hcsr04_get_distance(void) {
	if(hc_init != 1){
		return 0;
	}
	int has_echo = 0;
	gpio_set_level(TRIGGER, 1);
	ets_delay_us(100);
	gpio_set_level(TRIGGER, 0);
	gettimeofday(&tv, NULL);
	uint32_t startTime = tv.tv_usec;
	// Wait for echo to go high and THEN start the time
	while (gpio_get_level(ECHO) == 0 && gettimeofday(&tv, NULL) && (tv.tv_usec - startTime) < 500 * 1000) {
		// Do nothing;
	}
	gettimeofday(&tv, NULL);
	startTime = tv.tv_usec;
	while (gpio_get_level(ECHO) == 1 && gettimeofday(&tv, NULL) && (tv.tv_usec - startTime) < 500 * 1000) {
		has_echo = 1;
	}
	if (gpio_get_level(ECHO) == 0 && has_echo == 1) {
		gettimeofday(&tv, NULL);
		uint32_t diff = tv.tv_usec - startTime; // Diff time in uSecs
		// Distance is TimeEchoInSeconds * SpeedOfSound / 2
		float distance  = 340.29 * diff / (1000 * 1000 * 2); // Distance in meters
		return distance * 100;
	}
	return 0;
}
void hcsr04_init(int _TRIGGER, int _ECHO){
	TRIGGER = _TRIGGER;
	ECHO = _ECHO;
	gpio_pad_select_gpio(TRIGGER);
	gpio_pad_select_gpio(ECHO);
	gpio_set_direction(TRIGGER, GPIO_MODE_OUTPUT);
	gpio_set_direction(ECHO, GPIO_MODE_INPUT);
	hc_init = 1;
}
