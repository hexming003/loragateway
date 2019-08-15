#!/bin/sh
################################config ETH2########################################
MACADDR=86:43:C0:A8:01:E7                             
IPADDR=192.168.1.231                                 
NETMASK=255.255.255.0                                 
GW=192.168.1.1                                        
DEV=eth2

echo "mac=$MACADDR"
echo "ip=$IPADDR"
echo "netmask=$NETMASK"
echo "gw=$GW"

ifconfig $DEV down &&
sleep 1 &&
sync

/MeterRoot/TestTool/gpio -w PA28 0 &&
sleep 1 &&
sync
/MeterRoot/TestTool/gpio -w PA28 1 &&
sleep 2 &&
sync

ifconfig $DEV hw ether $MACADDR $DEV
ifconfig $DEV $IPADDR up
sleep 1 &&
sync
ifconfig $DEV netmask $NETMASK
#route add default metric 10 gw $GW dev $DEV
#ifconfig UP BROADCAST MULTICAST MTU:1500
route add -net 224.0.0.0 netmask 224.0.0.0 dev $DEV
sleep 2 &&
route add default gw $GW dev $DEV

##################################################################################

