SRC = prev play pause stop next

all:	convert_to_png
	echo


convert_to_png:	
	./convert.sh ${SRC}



xmmsicondir=$(datadir)/xfce4/xmms-plugin
xmmsicon_DATA=                                                          \
        xmms-plugin-prev.png                                            \
        xmms-plugin-play.png                                            \
        xmms-plugin-pause.png                                           \
        xmms-plugin-stop.png                                            \
        xmms-plugin-next.png

EXTRA_DIST = $(xmmsicon_DATA)

