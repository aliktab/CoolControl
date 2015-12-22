/*
  Cool Controller.

  Copyright (c) 2014 Purple Wolf Ltd. All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/


#include <PwCore.h>
#include <I2C.h>
#include <MCP9808.h>


// Define hardware pins.

const uint8_t PIN_FAN[] = { 11, 12 }; 


// Time constants

const int64_t CYCLE_PAUSE  = (int64_t)1000;   // Duration of signal calculation cycle.
const int64_t CHECK_LENGTH = (int64_t)10;      // Number of cycles per check.

const int64_t MAX_ON_TIME  = (int64_t)30 * 60 * 1000;      // Max time FAN is on.
const int64_t MAX_OFF_TIME = (int64_t)6 * 60 * 60 * 1000;  // Max time FAN is off.


// Fan control constants

const uint8_t FAN_STATE_OFF  = 0;
const uint8_t FAN_STATE_LOW  = 1;
const uint8_t FAN_STATE_HIGH = 2;

const float TEMPER_OFF   = 38.0;   // Temperature when fans should be switched off.
const float TEMPER_ON    = 42.8;   // Temperature when fans should be switched on.
const float TEMPER_NORM  = 40.0;   // Normal temperature when we don't need to switch fan at all.


// Global variables

MCP9808 t_sensor = MCP9808(0x18);

int64_t start_time = 0;
int64_t mode_time  = 0;

bool fan_switched_on = false;


// Initialization

void setup()
{
  // Initialize serial connection.
  Serial.begin(57600); 
  Serial.println("CoolControl.");

  // Initializeline pins mode, value, state and timer.
  for (int idx = 0; idx < sizeof(PIN_FAN) / sizeof(uint8_t); idx++)
  {
    pinMode(PIN_FAN[idx], OUTPUT);
    digitalWrite(PIN_FAN[idx], LOW);
  }

  // Initialize I2C interface.
  I2c.begin();
  I2c.setSpeed(true);
  I2c.timeOut(250);

  // Initialize temperatore sensors.
  if (!t_sensor.initialize())
    Serial.println("Couldn't initialize MCP9808 t0 sensor");

  // Initialize time system.
  start_time = millis();
  mode_time = 0;

  // Init fan flag
  fan_switched_on = false;
}


// Main Loop

void loop()
{
  // Update time system.
  if (millis() < start_time) // Skip bad cycle.
  {
    start_time = millis();
    return;
  }
  mode_time += millis() - start_time;
  start_time = millis();

  // Get temperature.
  float temperature = 0.0;
  for (int idx = 0; idx < CHECK_LENGTH; idx++)
  {
    temperature += t_sensor.read_temp_C() / (float)CHECK_LENGTH;
    delay(CYCLE_PAUSE);
  }

  // Check temperature and manage fans
  // if (!fan_switched_on && (temperature > TEMPER_ON || 
  //                          temperature > TEMPER_NORM && mode_time > MAX_OFF_TIME))
  if (!fan_switched_on && (temperature > TEMPER_ON))
  {
    // Switch fan on.
    for (int idx = 0; idx < sizeof(PIN_FAN) / sizeof(uint8_t); idx++)
      digitalWrite(PIN_FAN[idx], HIGH);

    // Reset mode time counter.
    mode_time = 0;

    // Set fan mode flag.
    fan_switched_on = true;
  }

  if (fan_switched_on && (temperature < TEMPER_OFF || 
                          temperature < TEMPER_NORM && mode_time > MAX_ON_TIME))
  {
    // Switch fan off.
    for (int idx = 0; idx < sizeof(PIN_FAN) / sizeof(uint8_t); idx++)
      digitalWrite(PIN_FAN[idx], LOW);

    // Reset mode time counter.
    mode_time = 0;

    // Set fan mode flag.
    fan_switched_on = false;
  }

  // Serrialize current data.
  char log_str[128];
  sprintf(log_str, "  %s time is %li:%02li:%02li, Temperature: ", 
      fan_switched_on ? "ON " : "OFF",
      (long)((mode_time / 3600000)     ),
      (long)((mode_time / 60000  ) % 60),
      (long)((mode_time / 1000   ) % 60)
    );
  Serial.print(log_str);
  Serial.println(temperature);
}




