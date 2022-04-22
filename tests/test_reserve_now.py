from paho.mqtt import client as mqtt_client
import pytest, pathlib, os, random, time
from xprocess import ProcessStarter
from pathlib import Path

def proj_root(cwd=os.getcwd()):
    proj_root = cwd
    if not Path(proj_root).exists():
        print("\nEVERET-CORE NOT FOUND!")
    if not Path(proj_root).name == Path("everest-core"):
        print("\nLaunch pytest from the \"everest-core\" directory")
    return Path(proj_root)


def everest_start(xprocess, launchpath, instance_name, client):
    class Starter(ProcessStarter):

        pattern = "Everest started!"
        terminate_on_interrupt = True

        args = [ launchpath ]

        def startup_check(self):
            return True

    success = Starter
    logfile = xprocess.ensure(instance_name, success)

    yield success

    xprocess.getinfo(instance_name).terminate()


@pytest.fixture
def get_proj_root(cwd=os.getcwd()):
    return proj_root(cwd)

@pytest.fixture
def client_data():
    cd = {}
    cd["client_id"] = f'python-mqtt-{random.randint(0, 1000)}'
    cd["broker"] = 'localhost'
    cd["port"] = 1883
    cd["username"] = ""
    cd["password"] = ""
    cd["disable"] = "/carsim/cmd/disable"
    cd["enable"] = "/carsim/cmd/enable"
    cd["start-topic"] = "/carsim/cmd/execute_charging_session"
    cd["start-payload"] = "/carsim/cmd/execute_charging_session"
    return cd

@pytest.fixture
def client(client_data):
    def on_connect(client, userdata, flags, rc):
        if rc == 0:
            print("Connected to MQTT Broker!")
        else:
            print("Failed to connect, return code %d\n", rc)

    cl = mqtt_client.Client(client_data["client_id"])
    cl.username_pw_set(client_data["username"], client_data["password"])
    cl.on_connect = on_connect
    cl.connect(client_data["broker"], client_data["port"])
    return cl

@pytest.fixture
def everest_instance(xprocess, get_proj_root, client):
    launchpath = get_proj_root / Path("run_sil-ocpp-complete-fixture.sh")
    yield from everest_start(xprocess, launchpath, "i1", client)

def subscribe(client: mqtt_client):
    def on_message(client, userdata, msg):
        print(f"Received `{msg.payload.decode()}` from `{msg.topic}` topic")

    client.subscribe(topic)
    client.on_message = on_message

def publish(client, topic, payload=None):
    msg_count = 0
    while msg_count < 3:
        result = client.publish(topic=topic, payload=payload)

        # result: [0, 1]
        status = result[0]
        if status == 0:
            print(f"Send `{payload}` to topic `{topic}`")
        else:
            print(f"Failed to send message to topic {topic}")
        msg_count += 1
        time.sleep(1)


def subscribe(client: mqtt_client):
    def on_message(client, userdata, msg):
        print(f"Received `{msg.payload.decode()}` from `{msg.topic}` topic")

    client.subscribe(topic)
    client.on_message = on_message


def test_basic(client_data, client, everest_instance):
    subscribe(client)
    client.loop_start()
    publish(client, topic=client_data["disable"])
    publish(client, topic=client_data["enable"])
    payload = "sleep 1;iec_wait_pwr_ready;sleep 1;draw_power_regulated 16,3;sleep 30;unplug"
    publish(client, topic=client_data["start-topic"], payload=payload)

