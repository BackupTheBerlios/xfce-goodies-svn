#!/usr/bin/env python

import gtk
import xfce4.panel
import dbus
import dbus.service
import dbus.glib
import gobject


class DBUSService(dbus.service.Object):
  
  def __init__(self, busName, objectPath="/DBUSService"):
    dbus.service.Object.__init__(self, busName, objectPath)

  @dbus.service.method("org.xfce.FileManager")
  def Launch(self, command, display):
    print command, display


class VervePythonPlugin(xfce4.panel.Plugin):

  def __init__(self):
    xfce4.panel.Plugin.__init__(self)

    self.eventBox = gtk.EventBox()
    self.add(self.eventBox)
    self.eventBox.show()

    self.add_action_widget(self.eventBox)

    self.label = gtk.Label("--")
    self.eventBox.add(self.label)
    self.label.show()

if __name__ == "__main__":
  sessionBus = dbus.SessionBus()
  name = dbus.service.BusName("org.xfce.FileManager", bus=sessionBus)
  service = DBUSService(name)

  #mainloop = gobject.MainLoop()
  #mainloop.run()
  
  plugin = VervePythonPlugin()
  plugin.connect("destroy", lambda x: gtk.main_quit())
  plugin.show()
  gtk.main()
