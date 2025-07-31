import json

from flask import Flask, jsonify, request, make_response
from flask_mqtt import Mqtt

app = Flask(__name__)

S2C_SET_STATE = 0
C2S_UPD_STATE = 1

app.config['MQTT_BROKER_URL'] = 'localhost'
app.config['MQTT_BROKER_PORT'] = 1883
app.config['MQTT_USERNAME'] = ''
app.config['MQTT_PASSWORD'] = ''
app.config['MQTT_KEEPALIVE'] = 15
app.config['MQTT_TLS_ENABLED'] = False

mqtt = Mqtt()
mqtt.init_app(app)

devices = {
    "dev1": {
        "minTemp": 0,
        "maxTemp": 100,
        "automated": False,
        "temp": 24,
        "power": False,
        "roomTemp": 27.3,
        "humidity": 45
    }
}

def send_mqtt_message(power, temp, clientID):
    if clientID in devices:
        mqtt.publish('S2C/' + clientID, json.dumps(
            {
                "msg": S2C_SET_STATE,
                "temp": temp,
                "power": power,
            }
        ))

@mqtt.on_connect()
def handle_connect(client, userdata, flags, rc):
    mqtt.subscribe('C2S/#')

@mqtt.on_message()
def handle_mqtt_message(client, userdata, message):
    topic = message.topic
    payload = message.payload.decode()
    
    dat = json.loads(payload)
    device = topic.split("/")[1]

    if dat["msg"] == C2S_UPD_STATE:
        if device not in devices:
            devices[device] = {
                "minTemp": 0,
                "maxTemp": 100,
                "automated": False,
            }
            
        devices[device]["temp"] = int(dat["temp"])
        devices[device]["power"] = bool(dat["power"])
        devices[device]["roomTemp"] = float(dat["roomTemp"])
        devices[device]["humidity"] = float(dat["humidity"])
        print(device + ":", devices[device])
        
        if devices[device]["automated"]:
            if (devices[device]["roomTemp"] < devices[device]["minTemp"]) and devices[device]["power"]:
                send_mqtt_message(False, devices[device]["temp"], device)
            elif (devices[device]["roomTemp"] > devices[device]["maxTemp"]) and (not devices[device]["power"]):
                send_mqtt_message(True, devices[device]["temp"], device)
    
    print(topic, payload)

# send data to esp
@app.route("/senddata/", methods=["POST"])
def send_data():
    json_input = request.get_json()
    for i in ["temp", "power", "clientID", "automated", "maxTemp", "minTemp"]:
        if i not in json_input:
            return jsonify({"status": "error", "message": "!DEV missing data"}), 400
    
    device = json_input["clientID"]
    
    if device in devices:
        send_mqtt_message(bool(json_input["power"]), int(json_input["temp"]), device)
        
        devices[device]["automated"] = bool(json_input["automated"])
        devices[device]["minTemp"] = float(json_input["minTemp"])
        devices[device]["maxTemp"] = float(json_input["maxTemp"])
    
        return jsonify({ "status": "ok" })
    
    return jsonify({ "status": "error", "message": "Device not found!" })


# get data from esp (cached)
@app.route("/getdata/", methods=["POST"])
def get_data():
    json_input = request.get_json()
    for i in ["clientID"]:
        if i not in json_input:
            return jsonify({"status": "error", "message": "!DEV missing data"}), 400
    
    device = json_input["clientID"]
    if device in devices:
        return jsonify(
            { 
                "status": "ok",
                "temp": devices[device]["temp"],
                "power": devices[device]["power"],
                "roomTemp": devices[device]["roomTemp"],
                "humidity": devices[device]["humidity"],
                "automated": devices[device]["automated"],
                "minTemp": devices[device]["minTemp"],
                "maxTemp": devices[device]["maxTemp"],
            }
        )
    
    return jsonify({ "status": "error", "message": "Device not found!" })

# main page
@app.route("/", methods=["GET", "POST"])
def main_page():
    return app.send_static_file("index.html")