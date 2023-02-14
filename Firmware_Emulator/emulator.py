# Copyright © 2023 Trystan A Larey-Williams
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0

# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# This script emulates the BSB firmware. Although the timing of the ESP32
# hardware is not considered, it is a decent functional facsimile.
# Subscribed topics (inputs to the Arduino) are maintained as state and
# printed to the console every second. Published topics (sensor outputs)
# generate random values and publish once per second. Mulitple instances
# of the script can be run in separate console windows to emulate more than
# one device, such as 'Hot Side' and 'Cold Side'.

# Like the firmware, SSL/TLS is used by default. The files ca.pem, client.crt 
# and client.key ar expected to be in the working directory. Password auth
# is not used by default for the MQTT broker.

# Run with the interface name argument, e.g python.exe .\emulator.py 'Hot Side'
# Exit the script via CTRL-C

from paho.mqtt import client as mqtt_client
import ssl, time, inspect, os, threading, sys, json, signal, schedule, random

broker = '192.168.86.214'
port = 8883 # SSL/TLS client port
username = ''
password = ''
clientName = None


def connect_mqtt():
    def on_connect(client, userdata, flags, rc):
        if rc == 0:
            print("Connected")
        else:
            print("Failed to connect, return code %d\n", rc)

    client = mqtt_client.Client(clientName)
    client.tls_set("ca.pem", "client.crt", "client.key", tls_version=ssl.PROTOCOL_TLSv1_2)
    client.tls_insecure_set(True)
    #client.username_pw_set(username, password)
    print("Connecting to broker ...")
    client.on_connect = on_connect
    client.connect(broker, port)
    return client


def publish(client, topic, msg):
    result = client.publish(topic, msg)
    status = result[0]
    if status != 0:
        print(f"Failed to send message to topic {topic}")


def subscribe(client, topic):
    def on_message(client, userdata, msg):
        if( msg.topic == 'BSB/Configure' ):
            initTopicMappings(msg.payload.decode())
        else:
            subscriptions[topic] = msg.payload.decode()       

    client.subscribe(topic)
    client.on_message = on_message


def run_continuously(interval=0.5):
    cease_continuous_run = threading.Event()

    class ScheduleThread(threading.Thread):
        @classmethod
        def run(cls):
            while not cease_continuous_run.is_set():
                schedule.run_pending()
                time.sleep(interval)

    continuous_thread = ScheduleThread()
    continuous_thread.start()
    return cease_continuous_run


subscriptions = {}
publishers = {}
def initTopicMappings( jdata ):
    global gClient
    data = json.loads(jdata)
    if( list(data.keys())[0] != clientName ):
        return;

    for obj in data[list(data.keys())[0]]:
        if( obj['Type'] == 'PWM' ):
            print('Subscribing to PWM',  obj['Topic'], 'to GPIO',  obj['GPIO'] )
            subscriptions[obj['Topic']] = 0
            subscribe(gClient, obj['Topic'])
        elif( obj['Type'] == 'Onewire' ):
            print('Publishing Onewire',  obj['Topic'], 'from sensor index',  obj['Index'] )
            publishers[obj['Topic']] = lambda : random.randint(50, 220)
        elif( obj['Type'] == 'Digital-In' ):
            print('Publishing to Digital-In',  obj['Topic'], 'from GPIO',  obj['GPIO'] )
            publishers[obj['Topic']] = lambda : random.randint(0, 1)
        elif( obj['Type'] == 'Digital-Out' ):
            print('Subscribing to Digital-Out',  obj['Topic'], 'to GPIO',  obj['GPIO'] )
            subscriptions[obj['Topic']] = 0
            subscribe(gClient, obj['Topic'])
        elif( obj['Type'] == 'Tilt' ):
            print('Publishing Tilt',  obj['Topic'], 'from sensor index',  obj['Index'] )
            publishers[obj['Topic']] = lambda : '{"Temp:"' + f'{random.randint(40, 90):.1f}' + ',"Grav:"' + f'{1.0 + (random.randint(10, 100) / 1000.0):.3f}' + '}'
        else:
            print( 'Unknown configuration object type ', obj )


gClient = None
stop_run_continuously = None

def handler(signum, frame):
    stop_run_continuously.set()
    gClient.disconnect()
    gClient.loop_stop()
    exit(1)


def run():
    global gClient
    global stop_run_continuously

    gClient = client = connect_mqtt()
    client.loop_start()
    time.sleep(1)

    signal.signal(signal.SIGINT, handler)

    # Send 'connected' ping to Node-Red every 3 seconds
    schedule.every(3).seconds.do( lambda client=client: publish(client, 'BSB/Online', clientName) )
    stop_run_continuously = run_continuously()

    subscribe( client, 'BSB/Configure' )
    publish( client, 'BSB/Register', clientName )
        
    while 1:
        for k, v in subscriptions.items():
            print(k, v)
        for k, v in publishers.items():
            publish(client, k, v())

        time.sleep( 1 )


if __name__ == '__main__':
    if( len(sys.argv) < 2 ):
        print( "Client name argument expected e.g. emulator.py 'Hot Side'" )
        exit()

    clientName = sys.argv[1]
    print( "Emulating %s", sys.argv[1] )
    run()