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
#include "esp_adc_cal.h"

int DO_CHANNEL;
int do_init = 0;
esp_adc_cal_characteristics_t do_characteristics;

// calibrate voltage to do value
float do37_calibrate(uint32_t voltage){
	float val = 0;
	if (voltage > 0 && voltage < 50){
		val = 8;
	}
	if (voltage >= 50 && voltage < 100){
		val = 9;
	}
	if (voltage >= 100){
		val = 10;
	}
	return val;
}

// Returns do meter from sensor
float do37_get_meter(void) {
	if(do_init != 1){
		return 0;
	}
	//int voltage = adc1_get_raw(DO_CHANNEL);
	uint32_t voltage = adc1_to_voltage(DO_CHANNEL, &do_characteristics);
	return do37_calibrate(voltage);
}

// Use adc1
void do37_init(uint32_t CHANNEL, uint32_t WIDTH, uint32_t ATTEN_DB){
	DO_CHANNEL = CHANNEL;
	gpio_pad_select_gpio(DO_CHANNEL);
	gpio_set_direction(DO_CHANNEL, GPIO_MODE_INPUT);
	adc1_config_width(WIDTH);
	adc1_config_channel_atten(DO_CHANNEL, ATTEN_DB);
	esp_adc_cal_get_characteristics(3300, ATTEN_DB, WIDTH, &do_characteristics);
	do_init = 1;
}
