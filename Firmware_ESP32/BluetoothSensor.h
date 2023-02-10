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

#ifdef ENABLE_TILT_SENSORS

#include <NimBLEDevice.h> // https://github.com/h2zero/NimBLE-Arduino

TaskHandle_t btSensorTask = NULL;
SemaphoreHandle_t xSemaphore = NULL;

struct TiltReading {
    unsigned short temp;
    float grav;
};

TiltReading tiltReadings[8];

void BTSensorScanTask( void* param ) {
    BLEScan* pBLEScan = (BLEScan*)param;
    pBLEScan->setActiveScan( true );
    pBLEScan->setInterval( 100 );
    pBLEScan->setWindow( 99 );

    while( 1 ) {
        BLEScanResults foundDevices = pBLEScan->start( 9, false );

        for( uint32_t i = 0; i < foundDevices.getCount(); ++i ) {
            BLEAdvertisedDevice device = foundDevices.getDevice(i);
            
            if( device.haveManufacturerData() == true ) {
                std::string strManufacturerData = device.getManufacturerData();
                unsigned int len = strManufacturerData.length();
                if( len > 64 )
                    continue;

                uint8_t cManufacturerData[64];
                strManufacturerData.copy( (char*)cManufacturerData, len, 0 );

                if( cManufacturerData[4] == 0xA4 && cManufacturerData[5] == 0x95 && cManufacturerData[6] == 0xBB ) {
                    // Found a Tilt
                    unsigned short indx = cManufacturerData[7] / 16 - 1;
                    if( indx < 8 ) {
                        xSemaphoreTake( xSemaphore, 200 / portTICK_PERIOD_MS );
                        tiltReadings[indx].temp = (cManufacturerData[20] << 8) | cManufacturerData[21];
                        tiltReadings[indx].grav = ((cManufacturerData[22] << 8) | cManufacturerData[23]) / 1000.0f; 
                        xSemaphoreGive( xSemaphore );        
                    }
                }
            }
        }

        pBLEScan->clearResults();
        vTaskDelay( 1000 / portTICK_PERIOD_MS );
    }
} 

void BTInit() {
    if( !btSensorTask ) {
        BLEDevice::init("");
        BLEScan* pBLEScan = BLEDevice::getScan();
        xSemaphore = xSemaphoreCreateMutex();
        xTaskCreatePinnedToCore( BTSensorScanTask, "BTS", 4096, pBLEScan, 1, &btSensorTask, 0 );
    }
}

#else

void BTInit() {

}

#endif

