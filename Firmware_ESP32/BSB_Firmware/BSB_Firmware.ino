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

// Intended to be used with Blue-Screen-Brewing⟨™⟩'s set of Node-Red nodes and flows for brewery automation.
// Simply edit the values in Config.h appropriately for your network and MQTT configuration.
// This Sketch has been developed and tested on a dual core ESP32-WROOM-UE32 board. Most other ESP32 boards will likely work.
// Other types of Arduino boards will almost certainly require modifications to the Sketch, as WiFi and Bluetooth libraries 
// tend to be hardware specific.
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///
/// You may need to install these packages from the library manager in the Arduino IDE in order to build.
/// Git repositories are provides in comments where helpful.
///

#include "Config.h"           // User configuration parameters

#include <time.h>             // Time functions to set currrent time for certificate validation

#include <WiFi.h>             // ESP32 Wifi
#ifdef USE_SECURE_TCP         // (see Config.h)
#include <WiFiClientSecure.h>
WiFiClientSecure client;
#else
WiFiClient client;
#endif

#include <ArduinoJson.h>      // JSON Parser
#include <MQTTPubSubClient.h> // https://github.com/hideakitai/MQTTPubSubClient

#include "OnewireSensor.h"    // OneWire temperature sensor support (optional see Config.h)
#include "BluetoothSensor.h"  // Blutooth - for Tilt devices        (optional see Config.h)

// File scope ///////////////////////////////////////////////////////////////////////////////
static std::function<void()> publishTopics[MAX_PUBLISH_TOPICS];
static unsigned short numPublishTopics = 0;
static arduino::mqtt::PubSubClient<MAX_PAYLOAD_SIZE> mqtt;
static unsigned short numPWMChannels = 0;
static const int MaxPWMDutyCycke = (int)(pow(2, PWM_RESOLUTION) - 1);
/////////////////////////////////////////////////////////////////////////////////////////////

// Globals //////////////////////////////////////////////////////////////////////////////////
const char consoleTopic[] = PSTR("BSB/Console");
/////////////////////////////////////////////////////////////////////////////////////////////

//
// Set internal clock on device.
//
void setClock() {
  configTime(3 * 3600, 0, PSTR("pool.ntp.org"), PSTR("time.nist.gov"));
}

//
// Stunningly, connects to WiFi.
//
void connectToWifi() {
  WiFi.mode( WIFI_STA );
  WiFi.begin( NETWORK_SSID, NETWORK_PASSWORD );

  while ( WiFi.status() != WL_CONNECTED ) {
    delay( 1000 );
  }
}

//
// Connects to MQTT broker.
//
void connectClient() {
  client.connect(MQTT_HOST, MQTT_PORT);
  if ( !client.connected() ) {
#ifdef USE_SECURE_TCP
    char buf[256];
    client.lastError(buf, 256);
    Serial.println( buf );
#endif
  }

  mqtt.begin(client);
  mqtt.connect(DEVICE_NAME);
}

//
// Receives a JSON payload from Node-Red describing how to map GPIO pins, Tilt sensors, Onewire
// sensors, etc. to MQTT messages. 
// 
void initTopicMappings( const String& payload ) {  
  DynamicJsonDocument doc(MAX_PAYLOAD_SIZE);
  DeserializationError err = deserializeJson(doc, payload);

  if ( err != DeserializationError::Ok ) {
    mqtt.publish( consoleTopic, PSTR("Failed to parse configuration. Check JSON.") );
    return;
  }

  JsonArray arr = doc.as<JsonArray>();
  numPublishTopics = 0;
  numPWMChannels = 0;

  for ( JsonObject mapping : arr ) {
    String type = mapping[PSTR("Type")];
    String topic = mapping[PSTR("Topic")];

    if ( numPublishTopics == MAX_PUBLISH_TOPICS ) {
      mqtt.publish( consoleTopic, PSTR("Exceeded max number of topics in configuration!") );
      break;
    }

    if ( type == PSTR("Digital-Out") ) {
      unsigned int GPIO = mapping[PSTR("GPIO")];
      pinMode(GPIO, OUTPUT);
      digitalWrite(GPIO, LOW);
      
      mqtt.subscribe(topic, [GPIO](const String & payload, const size_t size) {
        if ( payload.toInt() )
          digitalWrite(GPIO, HIGH);
        else
          digitalWrite(GPIO, LOW);
      });
    }
    else if ( type == PSTR("PWM") ) {
      unsigned int GPIO = mapping[PSTR("GPIO")];
      pinMode(GPIO, OUTPUT);
      ledcSetup(numPWMChannels++, PWM_FREQUENCY, PWM_RESOLUTION);
      ledcAttachPin(GPIO, numPWMChannels);
      
      mqtt.subscribe(topic, [numPWMChannels](const String & payload, const size_t size) {
        int dutyCyle = payload.toInt();
        if( dutyCyle > 0 && dutyCyle < MaxPWMDutyCycke )
          ledcWrite(numPWMChannels, payload.toInt());
      });
    }
    else if ( type == PSTR("Analog-In") ) {
      unsigned int GPIO = mapping[PSTR("GPIO")];
      pinMode(GPIO, INPUT);

      publishTopics[numPublishTopics++] = [GPIO, topic]() {
        static float lastValue = -1.0f;
        int val = analogRead( GPIO );
        if ( val != lastValue ) {
          mqtt.publish(topic, String(val));
          lastValue = val;
        }
      };
    }
    else if ( type == PSTR("Digital-In") ) {
      unsigned int GPIO = mapping[PSTR("GPIO")];
      pinMode(GPIO, INPUT);

      publishTopics[numPublishTopics++] = [GPIO, topic]() {
        static float lastValue = -1.0f;
        int val = digitalRead( GPIO );
        if ( val != lastValue ) {
          mqtt.publish(topic, String(val));
          lastValue = val;
        }
      };
    }
    else if ( type == PSTR("Counter") ) {
      unsigned int GPIO = mapping[PSTR("GPIO")];
      pinMode(GPIO, INPUT);

      publishTopics[numPublishTopics++] = [GPIO, topic]() {
        static unsigned int clk = millis();
        static unsigned long counter = 0;
        static int lastVal = 0;
        
        int current = digitalRead( GPIO );
        if( current > lastVal )
          ++counter;
        
        lastVal = current;
        
        if ( millis() - clk > 999 ) {
          mqtt.publish(topic, String(counter));
          clk = millis();
          counter = 0;
        }
      };
    }
    else if ( type == PSTR("Onewire") ) {
      unsigned int index = mapping[PSTR("Index")];
      
      publishTopics[numPublishTopics++] = [index, topic]() {
        static float lastValue = -1.0f;
        float temp = OnewireSensorByIndex( index ); // NOOP if ONE_WIRE_GPIO is not defined.
        if ( temp != lastValue ) {
          mqtt.publish(topic, String(temp));
          lastValue = temp;
        }
      };
    }
    else if ( type == PSTR("Tilt") ) {
      unsigned int index = mapping[PSTR("Index")];
      BTInit(); // NOOP if ENABLE_TILT_SENSORS is not defined.

      publishTopics[numPublishTopics++] = [index, topic]() {
        static float lastValue = -1.0f;
        if ( xSemaphoreTake( xSemaphore, 100 / portTICK_PERIOD_MS ) ) {
          unsigned short temp = tiltReadings[index].temp;
          float grav = tiltReadings[index].grav;
          xSemaphoreGive( xSemaphore );

          float val = temp + grav;
          if ( lastValue != val ) {
            String json("{\"Temp\":");
            json += String( temp );
            json += String(",\"Grav:\":");
            json += String( grav, 3 );
            json += String( "}" );
            mqtt.publish(topic, json);
            lastValue = val;
          }
        }
      };
    }
    else {
      publishTopics[numPublishTopics++] = [](){};
      mqtt.publish( consoleTopic, String(PSTR("Unknown 'type', ")) + type + PSTR("in configuration.") );
    }
  }
}

//
// Connect to WiFi and MQTT broker, optionally over a SSL/TLS connection. 
// Sends an initial 'Register' message to Node-Red to request the JSON
// configuration block used in initTopicMappings above.
//
void setup() {
  Serial.begin(115200);

  connectToWifi();
  Serial.println(WiFi.localIP());

  // Cerificate validate requires current time is set.
  setClock();

#ifdef USE_SECURE_TCP
  client.setCACert(ca_cert);
  client.setCertificate(client_cert);
  client.setPrivateKey(client_key);
  client.setInsecure(); // TODO: Fix. I think we need a fingerprint of the sever cert.
#endif

  connectClient();

  mqtt.subscribe(PSTR("BSB/Configure"), [](const String & payload, const size_t size) {
    initTopicMappings(payload);
  });

  mqtt.publish(PSTR("BSB/Register"), DEVICE_NAME);
}

//
// Update sensor and pin data and broadcast configured MQTT messages. 
// Send an 'Online' message occasionally to signal we're alive.
// Keep checking for a lost connection and reestablish if needed.
//
void loop() {
  if( WiFi.status() != WL_CONNECTED ) {
    connectToWifi();
    connectClient();
  }
  else if( !client.connected() ) {
    connectClient();
  }
  
  static unsigned int onLineTimer = millis();
  if( millis() - onLineTimer > 3000 ) {
    mqtt.publish(PSTR("BSB/Online"), DEVICE_NAME);
    onLineTimer = millis();
  }
    
  OnewirePoll();

  mqtt.update();

  for( unsigned short i = 0; i < numPublishTopics; ++i ) {
      publishTopics[i]();
  }
}
