from paho.mqtt import client as mqtt_client
import pytest, pathlib, os, random, time
from xprocess import ProcessStarter
from pathlib import Path
import subprocess

def proj_root(cwd=os.getcwd()):
    proj_root = cwd
    if not Path(proj_root).exists():
        print("\nEVERET-CORE NOT FOUND!")
    if not Path(proj_root).name == Path("everest-core"):
        print("\nLaunch pytest from the \"everest-core\" directory")
    return Path(proj_root)

def everest_start(xprocess, launchpath, instance_name, client, everest_data):
    class Starter(ProcessStarter):
        pattern = ".*void Everest::Everest::signal_ready().*Sending out module ready signal...*"
        terminate_on_interrupt = True
        max_read_lines = 2**10

        env = dict(os.environ)
        env['LD_LIBRARY_PATH'] = everest_data["lib-path"]
        args = [everest_data["manager_bin_path"], '--log_conf', everest_data["log"],
                '--main_dir', everest_data['dist'], '--schemas_dir', everest_data["schemas"], 
                '--conf', everest_data["config"], '--modules_dir', everest_data["modules"], 
                '--interfaces_dir', everest_data["interfaces"]]

        def startup_check(self):
            return True

    logfile = xprocess.ensure(instance_name, Starter)

    yield True

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
    cd["ready"] = "/everest/ready"
    return cd

@pytest.fixture
def everest_data(get_proj_root):
    ed = {}
    ed["everest-core"] = get_proj_root
    ed["build"] = ed["everest-core"] / Path("build")
    LD_LIBRARY_PATH = ed["build"] / Path("_deps/everest-framework-build/lib")
    ed["lib-path"] = LD_LIBRARY_PATH
    ed["manager_bin_path"] = ed["build"] / Path('dist/bin/manager')
    ed["config"] = ed["everest-core"] / Path("config/config-sil-ocpp.json")
    ed["log"] = ed["everest-core"] / Path("config/logging.ini")
    ed["dist"] = ed["build"] / Path("dist")
    ed["schemas"] = ed["dist"] / Path("schemas")
    ed["modules"] = ed["dist"] / Path("modules")
    ed["interfaces"] = ed["dist"] / Path("interfaces")
    return ed

def subscribe_transmitted_signals(cl, client_data):
    subscribe(cl, topic=client_data["disable"])
    subscribe(cl, topic=client_data["enable"])
    subscribe(cl, topic=client_data["start-topic"])

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
    subscribe(cl, topic=client_data["ready"])
    subscribe_transmitted_signals(cl, client_data)
    cl.loop_start()
    return cl

@pytest.fixture
def everest_instance(xprocess, get_proj_root, client, everest_data):
    launchpath = get_proj_root / Path("run_sil-ocpp-complete-fixture.sh")
    yield from everest_start(xprocess, launchpath, "i1", client, everest_data)

def publish(client, topic, payload=None):
    msg_count = 0
    status = 1
    while status == 1:
        result = client.publish(topic=topic, payload=payload)

        # result: [0, 1]
        status = result[0]
        if status == 0:
            print(f"Send `{payload}` to topic `{topic}`, try `{msg_count}`")
        else:
            print(f"Failed to send message to topic {topic}, try `{msg_count}`")
            time.sleep(0.1)
        
        msg_count += 1


def subscribe(client: mqtt_client, topic):
    def on_message(client, userdata, msg):
        print(f"Received `{msg.payload.decode()}` from `{msg.topic}` topic")

    client.subscribe(topic)
    client.on_message = on_message


def test_basic(client_data, client, everest_instance):

    publish(client, topic=client_data["disable"])
    time.sleep(0.001)
    publish(client, topic=client_data["enable"])
    time.sleep(1)
    payload = "sleep 1;iec_wait_pwr_ready;sleep 1;draw_power_regulated 16,3;sleep 30;unplug"
    publish(client, topic=client_data["start-topic"], payload=payload)
    # pybind11, expose: publish & call
    # API 