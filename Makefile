CC=g++
CFLAGS_SHOT= -W `pkg-config --cflags --libs opencv`  -Iinclude -lswscale -lavdevice -lavformat -lavcodec -lavutil -lswresample -lz -lconfig++ -w
CFLAGS_OCR= -W `pkg-config --cflags --libs opencv`  -Iinclude -lswscale -lavdevice -lavformat -lavcodec -lavutil -lswresample -lz -lconfig++ -ltesseract -w

PROGS_shot:= shot-boundary-detector view-shot-boundaries subshot-from-template
PROGS_tess:= tess-ocr-detector generate-mask 
PROGS_utils:= play-video

all: $(PROGS_shot) $(PROGS_tess) $(PROGS_utils)

ocr: $(PROGS_tess)

shots: $(PROGS_shot)

utils: $(PROGS_utils)


shot-boundary-detector: src/shots/shot-boundary-detector.cc 
	$(CC) src/shots/shot-boundary-detector.cc -o bin/$@  $(CFLAGS_SHOT)


view-shot-boundaries: src/shots/view-shot-boundaries.cc 
	$(CC) src/shots/view-shot-boundaries.cc -o  bin/$@  $(CFLAGS_SHOT)

subshot-from-template: src/shots/subshot-from-template.cc 
	$(CC) src/shots/subshot-from-template.cc -o  bin/$@  $(CFLAGS_SHOT)


tess-ocr-detector: src/ocr/tess-ocr-detector.cc 
	$(CC) src/ocr/tess-ocr-detector.cc -o  bin/$@  $(CFLAGS_OCR)


generate-mask: src/ocr/generate-mask.cc 
	$(CC) src/ocr/generate-mask.cc -o  bin/$@  $(CFLAGS_OCR)



play-video: src/utils/play-video.cc 
	$(CC) src/utils/play-video.cc -o  bin/$@  $(CFLAGS_SHOT)


clean:
	rm -rf  bin/*


mrproper: clean
	rm -rf  bin/*
