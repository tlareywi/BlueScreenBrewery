// Copyright © 2023 Trystan A Larey-Williams
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifdef ONE_WIRE_GPIO

#include <OneWire.h>
#include <DallasTemperature.h>

OneWire oneWire(ONE_WIRE_GPIO);
DallasTemperature sensors(&oneWire);

float OnewireSensorByIndex( unsigned short indx ) {
    float tempF = sensors.getTempFByIndex(indx);
    return roundf(tempF * 100) / 100.0f; // Round to 2 decimal places.
}

void OnewirePoll() {
    sensors.requestTemperatures();   
}

void OnewireBegin() {
    sensors.begin();
    mqtt.publish( consoleTopic, deviceId + PSTR(": Initializing Onewire, sensors detected ") + String(sensors.getDeviceCount()) );
}

#else

float OnewireSensorByIndex( unsigned short ) {
    return 0.0f;
}

void OnewirePoll() {

}

void OnewireBegin() {

}

#endif