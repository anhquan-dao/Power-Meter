import threading
import socket
from datetime import datetime
import time

def threaded(fn):
    def wrapper(*args, **kwargs):
        thread = threading.Thread(target=fn, args=args, kwargs=kwargs)
        thread.start()
        return thread
    return wrapper

class UDPServer():
    def __init__(self, database_, database_model, port_=4444):

        '''
         Start an udp server and bind it to the configured port
        '''
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

        self.server_config = ('0.0.0.0', port_)
        self.socket.bind(self.server_config)

        self.db = database_
        self.sensor = database_model

        self.f_lastRequestTime = time.time()
        self.shutdown = False

        self.thread_handle = self.rx_timer()

    def close(self):
        self.shutdown = True
        self.thread_handle.join()

    def __del__(self):
        self.close()

    def parser(self, data):
        '''
        The data is of the following format:
            data0_tag=data0&data1_tag=data1&[...]
        Move through the data looking for "=" and "&" 
        to determine the data tag and value
        '''

        # Return empty dict if data is not a string literal
        if type(data) != str:
            return dict()
        
        parse_dict = dict()
        while data.find("=") != -1:
            # Get the data tag
            eq_sign_idx = data.find("=")
            # Get the data value
            parse_dict[data[:eq_sign_idx]] = float(data[eq_sign_idx+1:data.find("&")])

            if(data.find("&") == -1):
                break
            
            # Truncate the data
            data = data[data.find("&")+1:]

        return parse_dict

    @threaded
    def rx_timer(self):
        while not self.shutdown:
            data = self.socket.recvfrom(1024)
            if data:
                #print received data
                data_string = data[0].decode("utf-8")
                # print('Client to Server: ' , data_string)
                parse_dict = self.parser(data_string)

                app_data = self.sensor(time=time.time(), current=parse_dict["current"], voltage=parse_dict["voltage"])
                self.db.session.add(app_data)
                self.db.session.commit()


if __name__ == "__main__":
    upd_timer = UDPServer()