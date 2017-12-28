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
#ifndef PH20_H_
#define PH20_H_

float ph20_calibrate(uint32_t VOLTAGE);
float ph20_get_meter(void);
void ph20_init(uint32_t CHANNEL, uint32_t WIDTH, uint32_t ATTEN_DB);

#endif
