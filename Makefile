# Build: make   (needs a C compiler and an X11 session at run time; no X11 dev headers needed)
dirt: dirt.c tint.c x11.c font8x8.c font_eu.c font_zh.c
	cc -O2 -Wall -o dirt dirt.c tint.c x11.c font8x8.c font_eu.c font_zh.c
clean:
	rm -f dirt demo
