#!/bin/sh
/etc/init.d/xencommons start
/etc/init.d/xend restart
sleep 1
#brctl addbr virbr0
virsh list 
sleep 5

#/home/doa/Document/xen
#virsh create /home/doa/Document/xen/VM2-ovs.xml
#virsh list
