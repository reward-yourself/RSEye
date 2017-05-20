all:
	gcc `pkg-config --cflags --libs xrender` -lm RSEye.c -o rseye

install:
	install -D rseye /usr/bin/rseye
	install -Dm644 rseye.1.gz /usr/share/man/man1/rseye.1.gz
	install -Dm644 LICENSE /usr/share/licenses/rseye-git/LICENSE
