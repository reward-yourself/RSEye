all:
	gcc `pkg-config --cflags --libs xrender` -lm RSEye.c -o rseye
