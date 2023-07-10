from flask import Flask, render_template, url_for, request, redirect, make_response
from flask_sqlalchemy import SQLAlchemy
import json

from flask_socketio import SocketIO

import requests

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


def send_sensor_config(url, data):
    headers = { 'User-Agent':   'Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:109.0) Gecko/20100101 Firefox/113.0',
                'Content-Length':   str(len(data))}
    
    response = requests.Response()
    try:
        response = requests.post(url, data=data, headers=headers)
    except requests.exceptions.ConnectionError:
        pass

class SensorConfiguration():

    def __init__(self, request_):

        self.params = { 'max_current'   : 1, \
                        'max_voltage'   : 12, \
                        'max_power'     : 100}

        if(type(request_) == type(request)):
            for key in self.params.keys():
                self.params[key] = float(request_.form.get(key, self.params[key]))

    def __str__(self):
        return str(self.params)

    def toHTTPData(self):
        HTTPData = str()
        for key in self.params.keys():
            HTTPData += "&{}={}".format(key, self.params[key])
        
        return HTTPData[1:]

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

        url = 'http://' + udp_server.sensor_address[0] + ':80/sensor_config'
        data = sensor_config.toHTTPData()
        print(url)
        t = Thread(target=send_sensor_config, args=(url, data))
        t.setDaemon(True)
        t.start()
        
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

@app.route('/stop',  methods = ["POST"])
def stop():
    if request.method == "POST":
        headers = { 'User-Agent':   'Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:109.0) Gecko/20100101 Firefox/113.0'}

        url = 'http://' + udp_server.sensor_address[0] + ':80/output'
        response = requests.Response()
        try:
            response = requests.post(url, headers=headers)
        except requests.exceptions.ConnectionError:
            pass

        return json.dumps({'success':True}), 200, {'ContentType':'application/json'} 
if __name__ == "__main__":
    db.create_all()
    socketio.run(app, host='0.0.0.0', port= 8090, debug=True, use_reloader=False)

    