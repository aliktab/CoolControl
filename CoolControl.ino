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
const int64_t CHECK_LENGTH = (int64_t)10;     // Number of cycles per check.
const int64_t T_LOG_TIME   = (int64_t)600000; // How often log temperature.

const int64_t MAX_ON_TIME  = (int64_t)30 * 60 * 1000;      // Max time FAN is on.
const int64_t MIN_ON_TIME  = (int64_t)2  * 60 * 1000;      // Min time FAN is on.

const int64_t MAX_OFF_TIME = (int64_t)6 * 60 * 60 * 1000;  // Max time FAN is off.


// Fan control constants

const uint8_t FAN_STATE_OFF  = 0;
const uint8_t FAN_STATE_LOW  = 1;
const uint8_t FAN_STATE_HIGH = 2;

const float TEMPER_OFF   = 30.0;   // Temperature when fans should be switched off.
const float TEMPER_NORM  = 34.0;   // Normal temperature when we don't need to switch fan at all.
const float TEMPER_ON    = 36.0;   // Temperature when fans should be switched on.


// Global variables

MCP9808 t_sensor = MCP9808(0x18);

int64_t start_time = 0;
int64_t mode_time  = 0;
int64_t save_time  = 0;
int64_t t_log_time = 0;

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
  mode_time  = 0;
  save_time  = 0;
  t_log_time = 0;

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
    temperature += t_sensor.read_temp_C();
    delay(CYCLE_PAUSE);
  }
  temperature /= (float)CHECK_LENGTH;

  // Check temperature and manage fans
  if (!fan_switched_on && (temperature > TEMPER_ON ||
                           temperature > TEMPER_NORM && mode_time > MAX_OFF_TIME))
  {
    // Switch fan on.
    for (int idx = 0; idx < sizeof(PIN_FAN) / sizeof(uint8_t); idx++)
      digitalWrite(PIN_FAN[idx], HIGH);

    // Reset mode time counter.
    save_time = mode_time;
    mode_time = 0;

    // Set fan mode flag.
    fan_switched_on = true;
  }

  if (fan_switched_on && (temperature < TEMPER_OFF  && mode_time > MIN_ON_TIME ||
                          temperature < TEMPER_NORM && mode_time > MAX_ON_TIME))
  {
    // Switch fan off.
    for (int idx = 0; idx < sizeof(PIN_FAN) / sizeof(uint8_t); idx++)
      digitalWrite(PIN_FAN[idx], LOW);

    // Reset mode time counter.
    save_time = mode_time;
    mode_time = 0;

    // Set fan mode flag.
    fan_switched_on = false;
  }

  // Serrialize current data.
  if (0 == mode_time) 
  {
    t_log_time = millis() - T_LOG_TIME;
    
    char log_str[256];
    sprintf(log_str, "  %s (%li:%02li:%02li) -> %s",
        fan_switched_on ? "OFF" : "ON ",
        (long)((save_time / 3600000)     ),
        (long)((save_time / 60000  ) % 60),
        (long)((save_time / 1000   ) % 60),
        fan_switched_on ? "ON " : "OFF"
      );
    Serial.println("");
    Serial.println("");
    Serial.print(log_str);
  }
  else
  {
    if (millis() - t_log_time > T_LOG_TIME)
    {
      t_log_time = millis();
      
      Serial.println("");
      Serial.print("    ");
      Serial.print(temperature);
      Serial.print(" ");
    }
    else
      Serial.print(".");
  }
}
