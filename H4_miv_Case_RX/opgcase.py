from digi.xbee.devices import XBeeDevice
import paho.mqtt.client as paho
from paho import mqtt
import time

if __name__ == '__main__':
    client = paho.Client(client_id="PythonClient_1")
    client.tls_set(tls_version=mqtt.client.ssl.PROTOCOL_TLS)
    client.username_pw_set("miha0876", "Badilli90") #hardcoded password should be changed.
    status = client.connect(host="3f9e723b074944aeaa0a7fa6b3d47978.s2.eu.hivemq.cloud", port=8883)

    device = XBeeDevice("/dev/ttyUSB0", 9600)
    device.open()

    for i in range(50):
        try:
            device.send_data_broadcast("Hello XBee World!")
            time.sleep(0.5)
            message = device.read_data()
            if message != None:
                print(message.remote_device)
                print(message.data)
                print(message.is_broadcast)
                print(message.timestamp)
                if status == 0:
                    # subscribe to all topics of encyclopedia by using the wildcard "#"
                    # client.subscribe("my/test/topic", qos=1)

                    # a single publish, this can also be done in loops, etc.
                    client.publish("case/personcount/topic", payload=message.data, qos=1)
        except:
            print("bad packet")
    """
    # loop_forever for simplicity, here you need to stop the loop manually
    # you can also use loop_start and loop_stop
    client.loop_forever()
    """

    client.disconnect()
    device.close()
