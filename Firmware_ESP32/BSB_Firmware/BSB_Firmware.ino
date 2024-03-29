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

#include "Config.h"           // User configuration parameters
#include "ISRs.h"             // Iterrrupt service routines for counters

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

// Globals //////////////////////////////////////////////////////////////////////////////////
const char consoleTopic[] = PSTR("BSB/Console");
String deviceId( DEVICE_NAME );
arduino::mqtt::PubSubClient<MAX_PAYLOAD_SIZE> mqtt;
/////////////////////////////////////////////////////////////////////////////////////////////

#include "OnewireSensor.h"    // OneWire temperature sensor support (optional see Config.h)
#include "BluetoothSensor.h"  // Blutooth - for Tilt devices        (optional see Config.h)
#include "AtlasSensor.h"

// File scope ///////////////////////////////////////////////////////////////////////////////
static std::function<void()> publishTopics[MAX_PUBLISH_TOPICS];
static unsigned short numPublishTopics = 0;
static String subscribeTopicStrings[MAX_SUBSCRIBE_TOPICS];
static std::function<void(const String&, const size_t)> subscribeTopics[MAX_SUBSCRIBE_TOPICS];
static unsigned short numSubscribeTopics = 0;
static unsigned short numPWMChannels = 0;
static const int MaxPWMDutyCycke = (int)(pow(2, PWM_RESOLUTION) - 1);
static unsigned short nextISR = 0;
static hw_timer_t *counterInterrupt = NULL;
/////////////////////////////////////////////////////////////////////////////////////////////

//
// Set internal clock on device.
//
void setClock() {
  configTime(0, 0, PSTR("pool.ntp.org"), PSTR("time.nist.gov"));
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
void connectClient( bool clean ) {
  client.connect(MQTT_HOST, MQTT_PORT);
  if ( !client.connected() ) {
#ifdef USE_SECURE_TCP
    char buf[256];
    client.lastError(buf, 256);
    Serial.println( buf );
#endif
  }

  mqtt.begin(client);
  mqtt.setCleanSession( clean );
  mqtt.connect(DEVICE_NAME);

  mqtt.subscribe(PSTR("BSB/Configure"), [](const String & payload, const size_t size) {
    initTopicMappings(payload);
  });

  if( !clean ) { // Resubscribe to topics if we're reconnecting.
    for( unsigned short i = 0; i < numSubscribeTopics; ++i )
      mqtt.subscribe(subscribeTopicStrings[i], subscribeTopics[i]);
  }
}

//
// Receives a JSON payload from Node-Red describing how to map GPIO pins, Tilt sensors, Onewire
// sensors, etc. to MQTT messages. 
// 
void initTopicMappings( const String& payload ) {  
  DynamicJsonDocument doc(MAX_PAYLOAD_SIZE);
  DeserializationError err = deserializeJson(doc, payload);

  if ( err != DeserializationError::Ok ) {
    mqtt.publish( consoleTopic, deviceId + PSTR(": Failed to parse configuration. Check JSON.") );
    return;
  }

  JsonObject::iterator itr = doc.as<JsonObject>().begin();
  if( deviceId != itr->key().c_str() ) {
    return;
  }

  mqtt.publish( consoleTopic, deviceId + PSTR(": Received new configuration.") );

  JsonArray arr = itr->value().as<JsonArray>();
  numPublishTopics = 0;
  numSubscribeTopics = 0;
  numPWMChannels = 0;
  nextISR = 0;

  for( JsonObject mapping : arr ) {
    String type = mapping[PSTR("Type")];
    String topic = mapping[PSTR("Topic")];

    if( numPublishTopics == MAX_PUBLISH_TOPICS ) {
      mqtt.publish( consoleTopic, deviceId + PSTR(": Exceeded max number of published topics in configuration!") );
      break;
    }
    if( numSubscribeTopics == MAX_SUBSCRIBE_TOPICS ) {
      mqtt.publish( consoleTopic, deviceId + PSTR(": Exceeded max number of subscribed topics in configuration!") );
      break;
    }

    if( type == PSTR("Digital-Out") ) {
      unsigned int GPIO = mapping[PSTR("GPIO")];
      bool activeLow = mapping[PSTR("Active-Low")];
      pinMode(GPIO, OUTPUT);
      if( activeLow )
        digitalWrite(GPIO, HIGH);
      else
        digitalWrite(GPIO, LOW);

      subscribeTopics[numSubscribeTopics] = [GPIO, activeLow](const String & payload, const size_t size) {
        if ( payload.toInt() )
          digitalWrite(GPIO, activeLow ? LOW : HIGH );
        else 
          digitalWrite(GPIO, activeLow ? HIGH : LOW );
      };
      
      subscribeTopicStrings[numSubscribeTopics] = topic;
      mqtt.subscribe(topic, subscribeTopics[numSubscribeTopics++]);
    }
    else if( type == PSTR("PWM") ) {
      unsigned int GPIO = mapping[PSTR("GPIO")];
      unsigned int channel = numPWMChannels++;
      pinMode(GPIO, OUTPUT);
      ledcSetup(channel, PWM_FREQUENCY, PWM_RESOLUTION);
      ledcAttachPin(GPIO, channel);
      
      subscribeTopics[numSubscribeTopics] = [channel](const String & payload, const size_t size) {
        int dutyCyle = payload.toInt();
        if( dutyCyle > -1 && dutyCyle <= MaxPWMDutyCycke )
          ledcWrite(channel, payload.toInt());
      };

      subscribeTopicStrings[numSubscribeTopics] = topic;
      mqtt.subscribe(topic, subscribeTopics[numSubscribeTopics++]);
    }
    else if( type == PSTR("Analog-In") ) {
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
    else if( type == PSTR("Digital-In") ) {
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
    else if( type == PSTR("Counter") ) {
      unsigned int GPIO = mapping[PSTR("GPIO")];
      unsigned short isr = nextISR++;
      if( isr >= MAX_COUNTERS )
      {
        mqtt.publish( consoleTopic, deviceId + PSTR(": Exceeded max number of counters!") );
        continue;
      }
      pinMode(GPIO, INPUT);
      attachInterrupt( digitalPinToInterrupt(GPIO), counterISRs[isr], FALLING );

      publishTopics[numPublishTopics++] = [counterSamples, isr, topic]() {
        if( counterSamples[isr] ) {
          mqtt.publish(topic, String(counterSamples[isr]));
          counterSamples[isr] = 0;
        }
      };
    }
    else if( type == PSTR("Onewire") ) {
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
    else if( type == PSTR("Atlas") ) {
      unsigned short rx = mapping[PSTR("Rx")];
      unsigned short tx = mapping[PSTR("Tx")];
      String commandTopic = mapping[PSTR("Command")];
      
      publishTopics[numPublishTopics++] = [topic, commandTopic, rx, tx]() {
        static AtlasSensor sensor( rx, tx, commandTopic );
        String data;
        if ( sensor.atlasPoll( data ) )
          mqtt.publish(topic, data);
      };
    }
    else if( type == PSTR("Tilt") ) {
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
            json += String(",\"Grav\":");
            json += String( grav, 3 );
            json += String( "}" );
            mqtt.publish(topic, json);
            lastValue = val;
          }
        }
      };
    }
    else {
      mqtt.publish( consoleTopic, deviceId + PSTR(": Unknown 'type', ") + type + PSTR(" in configuration.") );
    }
  }
}

void stopAll() { // Emergency stop of devices on lost broker connection.
  for( unsigned short i = 0; i < numSubscribeTopics; ++i )
      subscribeTopics[i]( String(PSTR("0")), 1 );
}

//
// Connect to WiFi and MQTT broker, optionally over a SSL/TLS connection. 
// Sends an initial 'Register' message to Node-Red to request the JSON
// configuration block used in initTopicMappings above.
//
void setup() {
  Serial.begin(115200);

  memset( (void*)counters, 0, sizeof( unsigned int ) * MAX_COUNTERS );
  memset( counterSamples, 0, sizeof( unsigned int ) * MAX_COUNTERS );

  connectToWifi();
  Serial.println(WiFi.localIP());

  // Cerificate validate requires current time is set.
  setClock();

#ifdef USE_SECURE_TCP
  client.setCACert(ca_cert);
  client.setCertificate(client_cert);
  client.setPrivateKey(client_key);
#endif

  connectClient( true /* clean session */  );

  OnewireBegin();

  // Setup timer interrupt for counters
  counterInterrupt = timerBegin( 0, TIMER_PRESCALER, true );
  timerAttachInterrupt( counterInterrupt, &counterSampler, true );
  timerAlarmWrite( counterInterrupt, 1000000, true );
  timerAlarmEnable( counterInterrupt );

  mqtt.publish( consoleTopic, deviceId + FW_VERSION );
  mqtt.publish(PSTR("BSB/Register"), DEVICE_NAME);
}

//
// Update sensor and pin data and broadcast configured MQTT messages. 
// Send an 'Online' message occasionally to signal we're alive.
// Keep checking for a lost connection and reestablish if needed.
//
void loop() {
  static unsigned long onLineTimer = millis();

  if( WiFi.status() != WL_CONNECTED ) {
    stopAll();
    connectToWifi();
    connectClient( false /* clean session */ );
  }
  else if( !client.connected() ) {
    stopAll();
    connectClient( false /* clean session */ );
  }
  else {
    if( millis() - onLineTimer > 3000 ) {
      mqtt.publish(PSTR("BSB/Online"), DEVICE_NAME);
      onLineTimer = millis();
    }
      
    OnewirePoll();

    mqtt.update();

    for( unsigned short i = 0; i < numPublishTopics; ++i )
        publishTopics[i]();
  }
}
