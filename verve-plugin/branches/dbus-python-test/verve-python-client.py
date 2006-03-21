#!/usr/bin/env python

import dbus
import gtk

bus = dbus.SessionBus()
remoteObject = bus.get_object("org.xfce.FileManager", "/DBUSService")
iface = dbus.Interface(remoteObject, "org.xfce.FileManager")
remoteObject.Launch("exo-open http://xfce.org", 1, dbus_interface = "org.xfce.FileManager")
