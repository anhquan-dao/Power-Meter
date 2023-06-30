import socket
import time

import random

# bind all IP
HOST = '0.0.0.0' 
# Listen on Port 
PORT = 4444
#Size of receive buffer   
BUFFER_SIZE = 1024    
# Create a TCP/IP socket
s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

rate = 0
avg_rate = 0
while True:
    rate = time.time()
    
    current = random.randint(0,1000)/100.0
    voltage = random.randint(0,1000)/100.0
    message = "current=" + str(f'{current:.4f}') + "&voltage=" + str(f'{voltage:.4f}')

    data = s.sendto(bytes(message, 'utf-8'), (HOST, PORT))
    time.sleep(0.1)

# Close connection
s.close()