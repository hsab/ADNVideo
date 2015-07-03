CC = gcc
OPTION = -I. 

all: run_lif_cleanocr
	
run_lif_cleanocr: run_lif_cleanocr.c charset.o lia_liblex.o manage_capital.o
	$(CC) $(OPTION) -o run_lif_cleanocr run_lif_cleanocr.c charset.o lia_liblex.o manage_capital.o

charset.o: charset.c
	$(CC) -c charset.c

lia_liblex.o: lia_liblex.c lia_liblex.h
	$(CC) -c lia_liblex.c

manage_capital.o: manage_capital.c
	$(CC) -c manage_capital.c

clean:
	rm run_lif_cleanocr charset.o lia_liblex.o manage_capital.o

