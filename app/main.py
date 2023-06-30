from flask import Flask, render_template, url_for, request, redirect, make_response
from flask_sqlalchemy import SQLAlchemy
import json

from flask_socketio import SocketIO

from threading import Thread
import socket
from datetime import datetime
import time

import random

from udp_server import UDPServer

app = Flask(__name__)
app.config["SQLALCHEMY_DATABASE_URI"] = 'sqlite:///test.db' # //// for absolute path
db = SQLAlchemy(app)

socketio = SocketIO(app)

class Sensor(db.Model):
    __tablename__ = 'sensor'
    id = db.Column(db.Integer, primary_key=True)
    current = db.Column(db.Float, default=0)
    voltage = db.Column(db.Float, default=0)
    time    = db.Column(db.Float, default=time.time())

    def __str__(self):
        return self.id

udp_server = UDPServer(db, Sensor)

class SensorConfiguration():

    def __init__(self, request_):

        self.params = {'shunt_resistor': 1, \
                       'max_current'   : 1, \
                       'max_voltage'   : 12}
        if(type(request_) == type(request)):
            for key in self.params.keys():
                self.params[key] = float(request_.form.get(key, self.params[key]))
        
        self.shunt_resistor = self.params['shunt_resistor']
        self.max_current    = self.params['max_current']
        self.max_voltage    = self.params['max_voltage']

    def __str__(self):
        return str(self.params)


@app.route('/', methods=['POST', 'GET'])
def index():
    if request.method == 'POST':
        pass

    else:
        
        return render_template("index.html")

@app.route('/config', methods=['GET', 'POST'])
def config():
    if request.method == 'POST':  
        sensor_config = SensorConfiguration(request)
        print(sensor_config)
        
        return ''

    else:
        return render_template("config.html")

@app.route('/about', methods=['GET', 'POST'])
def about():
    return render_template("about.html")

@app.route('/esp32_post',  methods = ["GET"])
def esp32_post():

    if request.method == 'GET':
        '''
            Upon the GET request, evaluate the amount of data
            needs to be sent based on interval between each request
        '''
        sensor_data_series = db.session.query(Sensor).\
            filter(Sensor.time > udp_server.f_lastRequestTime)

        sensor_data_list = sensor_data_series.order_by(Sensor.time).all()

        data = list()
        for i in range(len(sensor_data_list)):
            data.append([   sensor_data_list[i].time, \
                            sensor_data_list[i].current, \
                            sensor_data_list[i].voltage])

        response = make_response(json.dumps(data))
        response.content_type = 'application/json'

        udp_server.f_lastRequestTime = time.time()

        return response

if __name__ == "__main__":
    db.create_all()
    socketio.run(app, host='0.0.0.0', port= 8090, debug=True, use_reloader=False)

    