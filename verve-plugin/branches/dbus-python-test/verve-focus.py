#!/usr/bin/env python

import dbus
import gtk

bus = dbus.SessionBus()
remoteObject = bus.get_object("org.xfce.RunDialog", "/org/xfce/RunDialog")

dbus_iface = dbus.Interface(remoteObject, "org.xfce.RunDialog")

remoteObject.OpenDialog("/home/jannis/tmp", "1", dbus_interface = "org.xfce.RunDialog")
