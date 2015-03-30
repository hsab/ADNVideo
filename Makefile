CC=g++
CFLAGS=-W `pkg-config --cflags --libs opencv`  -Iinclude -lswscale -lavdevice -lavformat -lavcodec -lavutil -lswresample -lz -lconfig++ -lexpat -ltesseract 
PROGS_shot:= shot-boundary-detector view-shot-boundaries subshot-from-template
PROGS_tess:= tess-ocr-image tess-ocr-detector tess-ocr

all: $(PROGS_shot) $(PROGS_tess)

ocr: $(PROGS_tess)

shots: $(PROGS_shot)

shot-boundary-detector: src/shots/shot-boundary-detector.cc 
	$(CC) src/shots/shot-boundary-detector.cc -o bin/$@  $(CFLAGS)


view-shot-boundaries: src/shots/view-shot-boundaries.cc 
	$(CC) src/shots/view-shot-boundaries.cc -o  bin/$@  $(CFLAGS)

subshot-from-template: src/shots/subshot-from-template.cc 
	$(CC) src/shots/subshot-from-template.cc -o  bin/$@  $(CFLAGS)


tess-ocr-image: src/ocr/tess-ocr-image.cc 
	$(CC) src/ocr/tess-ocr-image.cc -o  bin/$@  $(CFLAGS)


tess-ocr-detector: src/ocr/tess-ocr-detector.cc 
	$(CC) src/ocr/tess-ocr-detector.cc -o  bin/$@  $(CFLAGS)


tess-ocr: src/ocr/tess-ocr.cc 
	$(CC) src/ocr/tess-ocr.cc -o  bin/$@  $(CFLAGS)




clean:
	rm -rf  bin/*


mrproper: clean
	rm -rf  bin/*
