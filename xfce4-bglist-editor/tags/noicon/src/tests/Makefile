all: test-autoscaleimage

test-autoscaleimage: test-autoscaleimage.c
	gcc -o test-autoscaleimage test-autoscaleimage.c -I.. ../xfceautoscaleimage.c ../aspectscale.c `pkg-config --cflags --libs gtk+-2.0`
	


