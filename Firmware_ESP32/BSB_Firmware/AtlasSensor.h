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

#ifdef MAX_ATLAS_SENSORS

#include <SoftwareSerial.h>

class AtlasSensor : public SoftwareSerial {
public:
    AtlasSensor( unsigned short id, unsigned short rx, unsigned short tx, const String& cal, const String& temp ) : SoftwareSerial( rx, tx ) {
        atlasId = id;
        begin(9600);
        if( !isListening() ) {
            mqtt.publish( consoleTopic, deviceId + PSTR(": Failed to initialize soft-serial on defined ports, check config.") );
            return;
        } 

        if( cal.length() > 0 ) {
            mqtt.subscribe( cal, [this](const String & payload, const size_t size) {
                if( payload )
                    print( payload + '\r' ); 
            });
        }

        if( temp.length() > 0 ) {
            mqtt.subscribe( temp, [this](const String & payload, const size_t size) {
                SetTempCompensation( payload.toFloat() );
            });
        }
    }

    bool atlasPoll( String& data ) {
        while( available() > 0 ) {
            char inchar = (char)read();

            if( inchar == '\r' ) {
                return true;
            }
            else
                data += inchar;
        }

        return false;
    }

private:
    unsigned short atlasId;   

    void SetTempCompensation( float tempC ) {
        char tempBuf[7];
        dtostrf( tempC, 0, 2, tempBuf ); 
        print( PSTR("T,") );
        print( tempBuf );           
        print( PSTR('\r') ); 
    }  
};

#else

class AtlasSensor {
public:
    void initAtlasDevice( unsigned short id, unsigned short rx, unsigned short tx ) {}
    void atlasPoll() {}
};

#endif



