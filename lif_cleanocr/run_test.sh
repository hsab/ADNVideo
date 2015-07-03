cat ocr/bfm-continous.ocr.resized.filtred.xml | ./src/run_lif_cleanocr -lex data/biglex_utf8.txt -confusion data/confusion.txt -threshold 40 
cat ocr/bfm-continous.ocr.resized.xml | ./src/run_lif_cleanocr -lex data/biglex_utf8.txt -confusion data/confusion.txt -threshold 40
cat ocr/bfm-continous.ocr_step2000.xml | ./src/run_lif_cleanocr -lex data/biglex_utf8.txt -confusion data/confusion.txt -threshold 40

