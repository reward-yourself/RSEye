all:
	gcc `pkg-config --cflags --libs xrender` -lm RSEye.c -o rseye

install:
	install -D rseye /usr/bin/rseye
