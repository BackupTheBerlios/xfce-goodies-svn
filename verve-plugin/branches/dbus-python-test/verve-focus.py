#!/usr/bin/env python

import os
import dbus
import gtk

bus = dbus.SessionBus()
remoteObject = bus.get_object("org.xfce.RunDialog", "/org/xfce/RunDialog")

dbus_iface = dbus.Interface(remoteObject, "org.xfce.RunDialog")

remoteObject.OpenDialog(os.environ.get("HOME"), "", dbus_interface = "org.xfce.RunDialog")
