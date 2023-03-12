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
 
 #include <pgmspace.h>

// Sent to the node-red configuation flow on startup. The flow maps this name
// to a definition of MQTT messages -> Arduino Pin assignments.
#define DEVICE_NAME      PSTR("Hot Side")

// The name of your Wifi network.
#define NETWORK_SSID     PSTR("MySSID")

// The password for your Wifi network.
#define NETWORK_PASSWORD PSTR("password")

// MQTT address and port (1883 is default, use 8883 for SSL)
#define MQTT_HOST         PSTR(IPAddress(192, 168, 86, 214))
//#define MQTT_HOST         "example.hostname.com"
#define MQTT_PORT         8883
// Largest payload accepted. Needs to be large enough to accomodate the JSON configuration blobs.
#define MAX_PAYLOAD_SIZE  1024 
// Maximum number of 'publish' topics that can be defined (i.e. number of distinct sensor or pin reads)
#define MAX_PUBLISH_TOPICS   24

// PWM Options. The resolution is compatible with a common Anoalog Amplifer board. 
#define PWM_RESOLUTION 8 // 0 - 255
#define PWM_FREQUENCY  2000

// Comment line below to disable one-wire temperature sensor (DS18B20) integration
#define ONE_WIRE_GPIO 5

// Comment line below to disable bluetooth (i.e. Tilt Hydrometer integration)
#define ENABLE_TILT_SENSORS

// Comment line below to disable Atlas sensor integration (pH, Dissolved O2) 
#define MAX_ATLAS_SENSORS 6

// Prescaler for the counter interrupt timer. 80 is appropraite for ESP32 boards.
#define TIMER_PRESCALER 80

// Comment line below to disable SSL/TLS. Only consider this for testing/prototyping.
#define USE_SECURE_TCP

#ifdef USE_SECURE_TCP
// Authority cert, client cert and client key for SSL/TLS. 
// These are only needed if USE_SECURE_TCP is enabled (highly recommended!)
// See generate-CA.sh at https://github.com/owntracks/tools/tree/master/TLS 
// to generate certs and keys that will work with the Mosquitto broker. 
  static const char ca_cert[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
-----END CERTIFICATE-----
)EOF";

static const char client_cert[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
-----END CERTIFICATE-----
)EOF";

static const char client_key[] PROGMEM = R"EOF(
-----BEGIN PRIVATE KEY-----
-----END PRIVATE KEY-----
)EOF";
#endif

