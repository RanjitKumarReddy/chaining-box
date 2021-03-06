#!/usr/bin/env python

from scapy.all import *
from scapy.contrib.nsh import NSH
import netifaces
import sys
import argparse

# def print_usage():
    # print '%s -i <interface> [-m <out dest MAC>] [-n] ' % sys.argv[0]

def main(argv):
    parser = argparse.ArgumentParser(prog=sys.argv[0],description='Generates packets with or without NSH')
    
    parser.add_argument('-i', '--iface', metavar='<out iface>', 
        help='Interface to send packet', required=True)

    parser.add_argument('-n', '--no-nsh', action='store_const', const=True,
        default=False,help='Send packet without NSH')

    parser.add_argument('-d', '--dmac', metavar='<dest MAC>', 
        default='ab:bc:cd:de:ef:fa', help='Destination MAC address')

    args = vars(parser.parse_args())

    with_nsh = not args['no_nsh'] 
    out_iface = args['iface']
    out_dest_mac = args['dmac']  
    out_src_mac = netifaces.ifaddresses(out_iface)[netifaces.AF_LINK][0]['addr']

    html  = """
    <html>
    <head>
        <title>Messages from the SFs</title>
    </head>
    <body>
    </body>
    </html>
    """

    http_resp  = "HTTP/1.1 200 OK\r\n"
    http_resp += "Server: exampleServer\r\n"
    http_resp += "Content-Length: " + str(len(html)) + "\r\n"
    http_resp += "\r\n"
    http_resp += html

    # Generate input packets
    i_eth = Ether(src="AA:BB:CC:DD:EE:FF",dst="FA:EA:DA:CA:BA:AA")
    i_ip = IP(src="10.10.10.10",dst="10.10.10.11")
    i_tcp = TCP(sport=1000,dport=2000)

    o_eth = Ether(src=out_src_mac,dst=out_dest_mac)
    extra_bytes = Raw("\x00\x00\x00\x00\x00\x00")
    if with_nsh:
        in_pkt = o_eth/Dot1Q(vlan=10)/NSH(NSP=0x1,NSI=0xFF,MDType=0x2)/extra_bytes/i_eth/i_ip/i_tcp/http_resp/Raw("\x00\x00\x00\x00\x00\x00")
    else:
        in_pkt = i_eth/i_ip/i_tcp/http_resp

    print "\nPacket size: %d\n" % len(in_pkt)
#    print "Beautiful packet!\n"
#    in_pkt.show()
    print "Ugly packet!\n"
    hexdump(in_pkt)

    # print out_iface
    sendp(in_pkt,iface=out_iface)
    # wrpcap('nsh.pcap',in_pkt)

if __name__ == "__main__":
   main(sys.argv)
