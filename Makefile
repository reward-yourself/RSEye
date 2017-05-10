all:
	gcc `pkg-config --cflags --libs xrender` -lm RSEye.c -o rseye

run:
	./rseye 2>/tmp/rseye.log&
