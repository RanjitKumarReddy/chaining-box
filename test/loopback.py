#!/usr/bin/env python

import struct
import sys,os
import socket
import binascii
import signal

def signal_handler(signal, frame):
    print "\n\n" + str(count) + " packets received"
    sys.exit(0)

signal.signal(signal.SIGINT, signal_handler)

iface = sys.argv[1]

rawSocket=socket.socket(socket.AF_PACKET,socket.SOCK_RAW,socket.htons(0x0800))
rawSocket.bind((iface,0))
sendSocket=socket.socket(socket.AF_PACKET,socket.SOCK_RAW,socket.htons(0x0800))

# print 'Buffer size: ' + str(rawSocket.getsockopt(socket.SOL_SOCKET,socket.SO_RCVBUF)) 
# rawSocket.setsockopt(socket.SOL_SOCKET,socket.SO_RCVBUF,4096)
# print 'Buffer size: ' + str(rawSocket.getsockopt(socket.SOL_SOCKET,socket.SO_RCVBUF)) 

count=0
# rawSocket.bind((sys.argv[1], socket.htons(0x0800)))
sendSocket.bind((sys.argv[1], socket.htons(0x0800)))
"Sniffing..."
while True:
    receivedPacket=rawSocket.recv(65536)
    try:
        sendSocket.send(receivedPacket)
    except Exception as e:
        pass
    else:
        count+=1
