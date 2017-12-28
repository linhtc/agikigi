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
#include "driver/adc.h"

int PH_CHANNEL;
int do_init = 0;

// calibrate voltage to ph value
float do37_calibrate(int voltage){
	float ph = 0;
	if (voltage >= 0 && voltage < 50){
		ph = 8;
	}
	if (voltage >= 50 && voltage < 100){
		ph = 9;
	}
	if (voltage >= 100){
		ph = 10;
	}
	return ph;
}

// Returns ph meter from sensor
float do37_get_meter(void) {
	if(do_init != 1){
		return 0;
	}
	int voltage = adc1_get_raw(PH_CHANNEL);
	return do37_calibrate(voltage);
}

// Use adc1
void do37_init(int CHANNEL, int WIDTH, int ATTEN_DB){
	PH_CHANNEL = CHANNEL;
	gpio_pad_select_gpio(PH_CHANNEL);
	gpio_set_direction(PH_CHANNEL, GPIO_MODE_INPUT);
	adc1_config_width(WIDTH);
	adc1_config_channel_atten(PH_CHANNEL, ATTEN_DB);
	do_init = 1;
}
