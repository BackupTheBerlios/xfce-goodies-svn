#!/usr/bin/env python
# Released under GNU Library General Public License 2

# TODO fix reordering
# TODO save file chooser current directory on disk
# TODO fix that: on delete-event check if unsaved, brag; if Cancel pressed, stay in !! <-- doesnt work
# TODO error dialog box on load if needed.
# TODO file chooser with preview too ;P
# TODO make image path in list unique
# TODO add directory, recurse

import pygtk
pygtk.require("2.0")
import gtk
import gobject
import string
import urllib
import os
from os.path import basename
import exceptions
import errno
import sys

os.chdir(os.environ["HOME"])

THUMB_WIDTH=128
THUMB_HEIGHT=128

TARGET_URI = 0
TARGET_STRING = 1

targets = [
        ("text/uri-list", 0, TARGET_URI),
        ("text/plain", 0, TARGET_STRING),
        ("UTF8_STRING", 0, TARGET_STRING),
        ("text/unicode", 0, TARGET_STRING), # netscape... todo: test
        ("STRING", 0, TARGET_STRING)
#       ("GTK_TREE_MODEL_ROW", gtk.TARGET_SAME_WIDGET, 0)
]

def _emptyPixbuf(w, h):
	xw = w
	xh = h
	if xw < 1: xw = 1
	if xh < 1: xh = 1
	pix = gtk.gdk.Pixbuf(gtk.gdk.COLORSPACE_RGB, True, 8, xw, xh)
	
	pix.fill(0x0)
	
	return pix

def hlabel(txt, w):
	h = gtk.HBox()
	h.pack_start(gtk.Label(txt), False, False)
	h.pack_start(w, False, False)
	#h.show_all()
	return h
	
def tree_path_prev(path):
	if path[-1] > 0:
		path = path[:-1] + (path[-1] - 1,)
		return path
	else:
		return None

def tree_path_next(path, model):
	if path[-1] < len(model) - 1: # sigh
		path = path[:-1] + (path[-1] + 1,)
		return path
	else:
		return None
	

def menuof(lst):
	m = gtk.Menu()
	for item in lst:
		mi = gtk.MenuItem(item)
		mi.show()
		m.append(mi)
		
	return m
	
def aspect_scale_simple(pix, mw, mh, mode):
	npix = _emptyPixbuf(mw, mh)

	if pix == None: return npix
	
	ow = pix.get_width()
	oh = pix.get_height()
	if mw <= 0 or mh <= 0 or ow <= 0 or oh <= 0:
		return npix
	
	if ow <= oh:
		# oh major
		
		nh = mh
		nw = ow * mh / oh
	else:
		# ow major
		nw = mw
		nh = oh * mw / ow

	if nh > mh:
		nh = mh
		nw = ow * mh / oh
		
	if nw > mw:
		nw = mw
		nh = oh * mw / ow
		
	if nw > mw: nw = mw
	if nh > mh: nh = mh
	
	n2pix = pix.scale_simple(nw, nh, mode)
	
	x = (mw - n2pix.get_width()) / 2
	y = (mh - n2pix.get_height()) / 2
	
	n2pix.copy_area(0, 0, nw, nh, npix, x, y)
	
	return npix

		
class My2CellRendererPixbuf(gtk.GenericCellRenderer):
	__gproperties__ = {
		"loaded": (gobject.TYPE_BOOLEAN,
			"if the file is loaded",
			"denotes if the file is loaded from path in pixbuf",
			False,
			gobject.PARAM_READWRITE),
		"path": (gobject.TYPE_STRING, 
			"path to file", 
			"filesystem path to the image file to be loaded on demand",
			"",  # default
			gobject.PARAM_READWRITE),
		"pixbuf": (gtk.gdk.Pixbuf,
			"pixbuf",
			"the current pixbuf, if any",
			gobject.PARAM_READWRITE),
		"iter": (gtk.TreeIter,
			"iter",
			"the current iter",
			gobject.PARAM_READWRITE)
	}
	
	def __init__(self):
		#gtk.CellRendererPixbuf.__init__(self)
		gtk.GenericCellRenderer.__init__(self)
		self.__path = None
		self.__width = None
		self.__height = None
		self.__loaded = False
		self.__iter = None
		self.__pix = None

	def setSize(self, width, height):
		self.__width = width
		self.__height = height
		self.__pix =_emptyPixbuf(width, height)
		
	def do_get_property(self, property):
		if property.name == "path":
			return self.__path
		elif property.name == "loaded":
			return self.__loaded
		elif property.name == "pixbuf":
			return self.__pix
		elif property.name == "iter":
			return self.__iter
		else:
			raise AttributeError, "unknown property %d" % property.name
			
	def do_set_property(self, property, value):
		if property.name == "path":
			self.__path = value
		elif property.name == "loaded":
			self.__loaded = value
		elif property.name == "pixbuf":
			self.__pix = value
		elif property.name == "iter":
			self.__iter = value
		else:
			raise AttributeError, "unknown property %d" % property.name

	def on_get_size(self, widget, cell_area):
		return 0, 0,self.__width, self.__height
		pass
		
	def _load(self, tree):
		#print "LOAD", self.__path
		try:
			pix = gtk.gdk.pixbuf_new_from_file(self.__path)
			if pix:
				interp = gtk.gdk.INTERP_NEAREST
				pix = aspect_scale_simple(pix, self.__width, self.__height, interp)
				
				self.set_property("pixbuf", pix)
	
			self.set_property("loaded", True)
			model = tree.get_model()
		finally:
			try:
				iter = self.__iter
				model.set_value(iter, 3, True)
				if pix: model.set_value(iter, 0, pix)
			except:
				pass
		
	def on_render(self, window, widget, background_area, cell_area, expose_area, flags):
		if not self.__loaded:
			self.__loaded = True
			try:
				self._load(widget)
			except:
				pass
			
		r = gtk.gdk.Rectangle(0, 0, self.__width, self.__height)
		
		#if not self.__loaded: self._load()
		gc = window.new_gc() # widget->style->black_gc

		r.x = r.x + cell_area.x
		r.y = r.y + cell_area.y
		r.width = r.width - 0 # xpad * 2
		r.height = r.height - 0 # ypad * 2
		
		draw_rect = cell_area.intersect(r)
		draw_rect = expose_area.intersect(draw_rect)
		# if intersect(cell_area, &pix_rct, &draw_rect) 
		# and intersect(expose_area, &draw_rect, &draw_rect):
		
		window.draw_pixbuf(gc, self.__pix, 
			draw_rect.x - r.x,
			draw_rect.y - r.y,
			draw_rect.x,
			draw_rect.y,
			draw_rect.width,
			draw_rect.height,
			gtk.gdk.RGB_DITHER_NONE, 0, 0)
		pass
		
	def on_activate(self, event, widget, path, background_area, cell_area, flags):
		pass

	def on_start_editing(self, event, widget, path, background_area, cell_area, flags):
		pass
		
gobject.type_register(My2CellRendererPixbuf)
		
class SImage(gtk.Image): # self-scaling image, on demand loading
	def __init__(self):
		self.origpixbuf = None
		gtk.Image.__init__(self)
		self.connect("size-allocate", self.size_allocate_cb)
		self.__size = (0,0)
		
	def size_allocate_cb(self, widget, alloc):
		#print "image size request"
		if alloc.width != self.__size[0] or alloc.height != self.__size[1]:
			pix = aspect_scale_simple(self.origpixbuf, alloc.width, alloc.height, gtk.gdk.INTERP_BILINEAR)
			self.__size = (alloc.width, alloc.height)
			self.set_from_pixbuf(pix)
		pass
		
	# size_allocate
		
	def set_orig_pixbuf(self, pix):
		self.origpixbuf = pix
		
	def get_orig_pixbuf():
		return self.origpixbuf
	pass

class BgListEditor(gtk.Dialog):
	C_THUMB = 0
	C_NAME = 1
	C_PATH = 2
	C_LOADED = 3
	C_ITER = 4
	def list_changed_cb(self, widget):
		sels = self.listTV.get_selection()
		#model, iter = sels.get_selected()
		path, column = self.listTV.get_cursor()
		if path:
			iter = self.listM.get_iter(path)
		else:
			iter = None
			
		model = self.listM
		
		if not iter:
			self.iL.set_text("")
			self.I.set_from_pixbuf(self.tinyPB)
			self.I.set_orig_pixbuf(self.tinyPB)
			
			return
			
		path = model.get_value(iter, self.C_PATH)

		try:		
			w, h = self.I.get_size_request()
			pix = gtk.gdk.pixbuf_new_from_file(path)
			self.I.set_orig_pixbuf(pix)
			pix = aspect_scale_simple(pix, w, h, gtk.gdk.INTERP_BILINEAR)
			self.I.set_from_pixbuf(pix)
			self._loadThumb(iter, update = True, good = True)
		except:
			pass
			
		self.iL.set_text(self.listM.get_value(iter, self.C_NAME))

	def _loadThumb(self, iter, update = False, good = False):
		global THUMB_WIDTH
		global THUMB_HEIGHT

		if self.listM.get_value(iter, self.C_LOADED) == True and not update: return
		
		path = self.listM.get_value(iter, self.C_PATH)
		pix = gtk.gdk.pixbuf_new_from_file(path)
		if pix:
			interp = gtk.gdk.INTERP_NEAREST
			if good == True: interp = gtk.gdk.INTERP_BILINEAR
			pix = aspect_scale_simple(pix, THUMB_WIDTH, THUMB_HEIGHT, interp)
		self.listM.set_value(iter, self.C_THUMB, pix)
		self.listM.set_value(iter, self.C_LOADED, True)

	def dropThumbs(self):
		global THUMB_WIDTH, THUMB_HEIGHT
		self.epix = _emptyPixbuf(THUMB_WIDTH, THUMB_HEIGHT)
		self.col0.set_fixed_width(THUMB_WIDTH + 5)
		iter = self.listM.get_iter_root()
		while iter:
			self.emptyThumb(iter)
			self.listM.set_value(iter, self.C_LOADED, False)
			iter = self.listM.iter_next(iter)
			
		self.listTV.hide()
		self.listTV.show()
		
	def x_cell_data_cb(self, column, renderer, store, iter):
		store.set_value(iter, self.C_ITER, iter) # FIXME works without?
		renderer.set_property("iter", iter)
			
	def add_clicked_cb(self, widget):
		fc = gtk.FileChooserDialog("Add Image...",
			None, gtk.FILE_CHOOSER_ACTION_OPEN,
			(gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL,
			gtk.STOCK_OPEN, gtk.RESPONSE_OK,
			"_Add Directory", gtk.RESPONSE_APPLY)
			)
		fc.set_default_response(gtk.RESPONSE_OK)
		
		filter = gtk.FileFilter()
		filter.set_name("Image Files")
		filter.add_pattern("*.jpg")
		filter.add_pattern("*.png")
		filter.add_pattern("*.jpeg")
		
		filter.add_mime_type("image/*")
		
		filter2 = gtk.FileFilter()
		filter2.set_name("All Files")
		filter2.add_pattern("*")

		fc.add_filter(filter)
		fc.add_filter(filter2)		
		
		if fc.run() == gtk.RESPONSE_OK:
			self.addImage(fc.get_filename())

		fc.destroy()
		del fc
		
	def remove_clicked_cb(self, widget):
		sels = self.listTV.get_selection()
		model, paths = sels.get_selected_rows()
		paths.reverse()
		p = None
		for path in paths:
			p = path
			iter = self.listM.get_iter(path)
			self.listM.remove(iter)
			
		if p != None:
			sels.unselect_all()
			sels.select_path(p)

		self._changed()
		
	def listm_swap(self, iter1, iter2):
		if not iter1: return
		if not iter2: return
		if iter1 == iter2: return

		a = []
		b = []
		
		nc = self.listM.get_n_columns()
		for i in range(nc):
			a.append(self.listM.get_value(iter1, i))
			b.append(self.listM.get_value(iter2, i))

		for i in range(nc):
			self.listM.set_value(iter1, i, b[i])
			self.listM.set_value(iter2, i, a[i])
			
		pass
		
	def up_clicked_cb(self, widget):
		curpath, curcolumn = self.listTV.get_cursor()
		movecursor = False
		sels = self.listTV.get_selection()
		model, paths = sels.get_selected_rows()
	
		nselps = []	
		for path in paths:
			if curpath == path:
				movecursor = True
				
			ppath = tree_path_prev(path)
			
			iter = self.listM.get_iter(path)
			if ppath:
				iterP = self.listM.get_iter(ppath)
				if iterP and iter:
					nselps.append(ppath)
					self._changed()
					self.listm_swap(iter, iterP)

		if movecursor == True:
			path = tree_path_prev(curpath)
			if path: self.listTV.set_cursor(path, None, False)
				
		sels.unselect_all()
		for nselp in nselps:
			sels.select_path(nselp)

		if len(nselps) == 0:
			sels.select_path(curpath)

			
		
	def down_clicked_cb(self, widget):
		curpath, curcolumn = self.listTV.get_cursor()
		movecursor = False
		sels = self.listTV.get_selection()
		model, paths = sels.get_selected_rows()
		
		paths.reverse()
		nselps = []
		for path in paths:
			if curpath == path:
				movecursor = True

			ppath = tree_path_next(path, self.listM)
			if not ppath: continue
			
			iter = self.listM.get_iter(path)
			iterP = self.listM.get_iter(ppath)
			if iterP and iter:
				nselps.append(ppath)
				self._changed()
				self.listm_swap(iter, iterP)

		if movecursor == True:
			path = tree_path_next(curpath, self.listM)
			if path: self.listTV.set_cursor(path, None, False)
			

		sels.unselect_all()
		for nselp in nselps:
			sels.select_path(nselp)
			
		if len(nselps) == 0:
			sels.select_path(curpath)

		
	def _changed(self, did = True):
		if did == False:
			self.set_title(self.__title % basename(self.fname))
			self.__changed = False
			self.set_default_response(gtk.RESPONSE_CANCEL)
			self.okB.set_sensitive(False)
			return

		if self.__changed == False:
			self.__changed = True
			self.okB.set_sensitive(True)
			self.set_default_response(gtk.RESPONSE_OK)
		
	def rows_reordered_cb(self, model, path, iter, arg):
		self._changed()
		pass
		
		
	def list_view_changed_cb(self, widget):
		global THUMB_WIDTH
		global THUMB_HEIGHT
		i = widget.get_history()
		if i <= -1: return
		
		sz = widget.get_data("keys")[i]
		THUMB_WIDTH = sz
		THUMB_HEIGHT = sz
		self.dropThumbs()
		lst = [x for x in self.gtkToPythonList()]
		self.pythonToGtkList(lst)
	
	def response_cb(self, widget, resp):
		self.__response = resp
			
	def delete_event_cb(self, widget, event):
		if self.__response != gtk.RESPONSE_CANCEL:
			if self.__changed == True:
				xfname = self.fname
				if xfname == None: xfname = "unnamed"
				md = gtk.MessageDialog(self, gtk.DIALOG_MODAL|gtk.DIALOG_DESTROY_WITH_PARENT, 
				gtk.MESSAGE_QUESTION, 
				gtk.BUTTONS_NONE, "Do you want to save changes in %s ?" % xfname)
				md.add_buttons(
					gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL,
					gtk.STOCK_NO, gtk.RESPONSE_NO,
					gtk.STOCK_SAVE, gtk.RESPONSE_OK
				)
				md.set_title(self.get_title() + " - Save")
				md.set_default_response(gtk.RESPONSE_OK)
				
				res = md.run()
				
				if res == gtk.RESPONSE_OK:
					self.save()
					
				md.destroy()
				del md
				
				if res == gtk.RESPONSE_CANCEL:
					#self.emit_stop_by_name("delete-event")
					return True #False
		
		return False
		
	def drag_data_received_cb(self, widget, context, x, y, data, info, timestamp):
		#self.listTV.get
		path = self.listTV.get_drag_dest_row()
		
		ipath = data.data
	
		if ipath.find("\n"):
			fpaths = string.split(ipath, "\n")
		else:
			fpaths = [ipath]
			
		for fpath in fpaths:
			if fpath.endswith("\r"): fpath = fpath[:-1]

			if fpath.startswith("file://"):
				fpath = fpath[7:]
				i = fpath.find("/")
				if i > -1:
					fpath = fpath[i:]
				
				fpath = urllib.unquote(fpath)
				
				if fpath != "":
					self.addImage(fpath, changed=True,insert=path)
		
		
	def _init_pointers(self):
		self.__pointers = { 
			"wait": gtk.gdk.Cursor(gtk.gdk.WATCH),
			"normal": gtk.gdk.Cursor(gtk.gdk.LEFT_PTR)
		}
		
	def setPointer(self, name):
		if self.window: self.window.set_cursor(self.__pointers[name])
		
		for i in range(10):
			gtk.main_iteration()
		
	def __init__(self):
		gtk.Dialog.__init__(self)
		
		self._init_pointers()
		global THUMB_WIDTH, THUMB_HEIGHT
		
		self.pathUnique = {}
		self.__response = None
		self.connect("delete-event", self.delete_event_cb)
		self.connect_after("response", self.response_cb)
		
		self.__fname = None
		self.__changed = False
		
		self.__title = "Background List Editor - %s"
		self.set_title(self.__title % "No List")
		
		self.epix = _emptyPixbuf(THUMB_WIDTH, THUMB_HEIGHT)
		self.tinyPB = _emptyPixbuf(1, 1)

		self.I = SImage()
		self.I.show()
		self.I.set_size_request(400, 400)
		
		
		self.listTV = gtk.TreeView()
		self.listTV.set_reorderable(True)

		cell0 = My2CellRendererPixbuf()
		cell0.setSize(THUMB_WIDTH, THUMB_HEIGHT)
		col0 = gtk.TreeViewColumn("", cell0, pixbuf=self.C_THUMB, path=self.C_PATH, loaded=self.C_LOADED)
		col0.set_cell_data_func(cell0, self.x_cell_data_cb)
		self.listTV.append_column(col0)
	
		cell1 = gtk.CellRendererText()
		col1 = gtk.TreeViewColumn("Name", cell1, text=self.C_NAME)
		self.listTV.append_column(col1)
		self.listTV.get_selection().set_mode(gtk.SELECTION_MULTIPLE)
		
		self.listM = gtk.ListStore(gtk.gdk.Pixbuf, gobject.TYPE_STRING, gobject.TYPE_STRING, gobject.TYPE_BOOLEAN, gtk.TreeIter)

		self.listTV.set_headers_visible(True) # FIXME false ?
		
		self.listTV.connect("drag-data-received", self.drag_data_received_cb)
		
		self.listTV.enable_model_drag_dest(targets, gtk.gdk.ACTION_DEFAULT)
		
		self.col0 = col0

		if True:
			col0.set_fixed_width(THUMB_WIDTH + 5)
			col0.set_sizing(gtk.TREE_VIEW_COLUMN_FIXED)
			col1.set_fixed_width(200)  # FIXME
			col1.set_sizing(gtk.TREE_VIEW_COLUMN_FIXED)
			self.listTV.set_property("fixed-height-mode", True)
			#self.listTV.set_fixed_height_mode(True)
			
		self.listTV.set_model(self.listM)

		self.listTV.get_selection().connect("changed", self.list_changed_cb)
		self.listTV.show()
		self.listS = gtk.ScrolledWindow()
		self.listS.set_policy(gtk.POLICY_NEVER, gtk.POLICY_ALWAYS)
		self.listS.add(self.listTV)
		self.listS.show()
		
		ag = gtk.AccelGroup()
		
		self.addB = gtk.Button("_Add ...")
		self.addB.add_accelerator("clicked", ag, gtk.gdk.keyval_from_name("Insert"), 
			0, gtk.ACCEL_VISIBLE)
			
		self.addB.connect("clicked", self.add_clicked_cb)
		self.removeB = gtk.Button("_Remove")
		self.removeB.add_accelerator("clicked", ag, gtk.gdk.keyval_from_name("Delete"), 
			0, gtk.ACCEL_VISIBLE)

		self.removeB.connect("clicked", self.remove_clicked_cb)
		self.upB = gtk.Button("_Up")
		self.upB.add_accelerator("clicked", ag, gtk.gdk.keyval_from_name("Up"), 
			gtk.gdk.SHIFT_MASK, gtk.ACCEL_VISIBLE)

		self.upB.connect("clicked", self.up_clicked_cb)
		self.downB = gtk.Button("_Down")
		self.downB.add_accelerator("clicked", ag, gtk.gdk.keyval_from_name("Down"), 
			gtk.gdk.SHIFT_MASK, gtk.ACCEL_VISIBLE)

		self.downB.connect("clicked", self.down_clicked_cb)
		
		self.add_accel_group(ag)
		
		self.listMB = gtk.VButtonBox()
		self.listMB.set_spacing(7)
		self.listMB.set_border_width(7)
		self.listMB.set_layout(gtk.BUTTONBOX_START)
		self.listMB.pack_start(self.addB, False, False)
		self.listMB.pack_start(self.removeB, False, False)
		self.listMB.pack_start(self.upB, False, False)
		self.listMB.pack_start(self.downB, False, False)
		self.listMB.show_all()
		
		self.h = gtk.HPaned() #Box()
		
		self.listOM = gtk.OptionMenu()
		self.listOM.set_data("keys", [256, 128, 64, 32])
		self.listOM.set_menu(menuof(["Big", "Normal", "Small", "Tiny"]))
		self.listOM.set_history(1)
		self.listOM.connect("changed", self.list_view_changed_cb)
		self.listOM.show()
		
		self.listV = gtk.VBox()
		self.listV.pack_start(hlabel("Thumbnail Size:", self.listOM), False, False)
		self.listV.pack_start(self.listS, True, True)
		self.listV.show()

		self.listH = gtk.HBox()
		self.listH.pack_start(self.listV, True, True)
		self.listH.pack_start(self.listMB, False, False)
		self.listH.show()
		
		self.iL = gtk.Label("")
		self.iL.show()
		
		self.iV = gtk.VBox()
		self.iV.set_spacing(7)
		self.iV.set_border_width(5)
		self.iV.pack_start(self.iL, False, False)
		self.iV.pack_start(self.I, True, True)
		self.iV.show()
		
		self.h.pack1(self.listH, True, True)
		self.h.pack2(self.iV, True, True)
		
		#self.h.set_spacing(7)
		self.h.set_border_width(7)
		self.h.show()
		self.vbox.pack_start(self.h)
		
		self.add_button(gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL)
		self.set_default_response(gtk.RESPONSE_CANCEL)
		self.okB = self.add_button(gtk.STOCK_OK, gtk.RESPONSE_OK)
		self.okB.set_sensitive(False)
		
		self.listM.connect("rows-reordered", self.rows_reordered_cb)

	def emptyThumb(self, iter):
		self.listM.set_value(iter, self.C_THUMB, self.epix)

	def addImage(self, path, changed=True, insert=None):
		if path in self.pathUnique:
			return None
		
		self.pathUnique[path] = 1
			
		if not insert:
			iter = self.listM.append(None)
		else:
			iter = self.listM.insert_before(None, self.listM.get_iter(insert))

		self.listM.set_value(iter, self.C_NAME, basename(path))
		self.listM.set_value(iter, self.C_PATH, path)
		self.listM.set_value(iter, self.C_LOADED, False)
		self.emptyThumb(iter)
		if changed == True:
			self._changed()
		return iter
		
	def clear(self):
		self.listM.clear()
		self.pathUnique = {}

	def load(self, fname):
		try:
			self.setPointer("wait")
			self.xload(fname)
		finally:
			self.setPointer("normal")
					
	def xload(self, fname):
		self.clear()
		
		line = os.popen("xfce-get-background", "r").readline()
		
		if line.endswith("\r"): line = line[:-1]
		if line.endswith("\n"): line = line[:-1]
		if line.endswith("\n"): line = line[:-1]
		if line.endswith("\r"): line = line[:-1]

		selitem = line
		
		try:
			for line in file(fname, "r"):
				if line.startswith("#"): continue
				
				if line.endswith("\r"): line = line[:-1]
				if line.endswith("\n"): line = line[:-1]
				if line.endswith("\n"): line = line[:-1]
				if line.endswith("\r"): line = line[:-1]

				if line == "": continue

				iter = self.addImage(line, False)
				
				if selitem == line:
					self.listTV.get_selection().unselect_all()
					self.listTV.get_selection().select_iter(iter)
					path = self.listM.get_path(iter)
					self.listTV.scroll_to_cell(path, None, True, 0.5, 0)
		except:
			pass

		self.fname = fname
		self._changed(False)

	def pythonToGtkList(self, lst):
		tpath, column = self.listTV.get_cursor()
		path = None
		if tpath: path = self.listM.get_value(self.listM.get_iter(tpath), self.C_PATH)
				
		self.clear()

		for cpath in lst:
			iter = self.addImage(cpath, False)
			if cpath == path:
				sels = self.listTV.get_selection()
				sels.unselect_all()
				sels.select_iter(iter)
				tpath = self.listM.get_path(iter)
				self.listTV.set_cursor(tpath, None, False)
		
	def gtkToPythonList(self):
		iter = self.listM.get_iter_root()
		while iter:
			path = self.listM.get_value(iter, self.C_PATH)
			yield path
			iter = self.listM.iter_next(iter)
				
	def save(self, fname = None):
		if fname == None: fname = self.fname
		if fname == None: return
		
		f = file(fname + ".tmp", "w")
		f.write("# xfce backdrop list")
		for path in self.gtkToPythonList():
			f.write("\n" + path)

		f.close()
		
		os.rename(fname + ".tmp", fname)
	
			
	# def checkalliamges
	# def addDir, Recursive

ble = BgListEditor()
ble.connect("destroy", lambda x: gtk.main_quit())
ble.show_now()
try:
	fname = sys.argv[1]
except:
	fname = os.path.join(os.environ["HOME"], ".xfce4", "bg-list")

try:
	ble.load(fname)
except exceptions.OSError, e:
	if e.errno != errno.ENOENT:
		raise # TODO error dialog box

if ble.run() == gtk.RESPONSE_OK:
	ble.save()
	
		
