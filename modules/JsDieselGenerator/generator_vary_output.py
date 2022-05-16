import paho.mqtt.publish as publish # add with: python3 -m pip install paho-mqtt
import time
import json
import datetime
import uuid

HOST = "localhost"

message_payload = {
	"data": {
		"args": {
			"limits_export": {
				"request_parameters": {
					"ac_current_A": {
						"current_A": 15
					}
				},
				"valid_until": "2022-09-12T05:36:24.335Z"
			},
			"limits_import": {},
			"schedule_export": [],
			"schedule_import": [],
			"uuid": "diesel_generator"
		},
		"id": "6cd97360-be87-45df-92dc-cf49eabf61cf",
		"origin": "energy_manager"
	},
	"name": "enforce_limits",
	"type": "call"
}


#######################################################################################################
if __name__ == '__main__':
	while 42:
		for i in range(31):
			timestamp_now = datetime.datetime.now(datetime.timezone.utc)
			timestamp_valid_until = timestamp_now + datetime.timedelta(seconds=10)
			message_payload['data']['args']['limits_export']['valid_until'] = timestamp_valid_until.isoformat("Z").replace('+00:00','Z')
			
			message_payload['data']['id'] = str(uuid.uuid4())
			
			message_payload['data']['args']['limits_export']['request_parameters']['ac_current_A']['current_A'] = i
			publish.single(topic="everest/diesel_generator/main/cmd", payload=json.dumps(message_payload), hostname=HOST)
			time.sleep(3)
			
		for i in range(31):
			timestamp_now = datetime.datetime.now(datetime.timezone.utc)
			timestamp_valid_until = timestamp_now + datetime.timedelta(seconds=10)
			message_payload['data']['args']['limits_export']['valid_until'] = timestamp_valid_until.isoformat("Z").replace('+00:00','Z')
			
			message_payload['data']['id'] = str(uuid.uuid4())
			
			message_payload['data']['args']['limits_export']['request_parameters']['ac_current_A']['current_A'] = 32 - i
			publish.single(topic="everest/diesel_generator/main/cmd", payload=json.dumps(message_payload), hostname=HOST)
			time.sleep(3)
			