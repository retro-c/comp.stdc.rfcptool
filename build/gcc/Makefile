./bin/rfcptool: ./tmp/rfcptool_compile.o ./tmp/rfcptool_cpspec.o ./tmp/rfcptool_decode.o ./tmp/rfcptool_encode.o ./tmp/rfcptool_file.o ./tmp/rfcptool_main.o ./tmp/rfcptool_native.o ./tmp/rfcptool_util.o ./tmp/rfcptool_verify.o
	mkdir -p ./bin
	gcc -o ./bin/rfcptool ./tmp/rfcptool_compile.o ./tmp/rfcptool_cpspec.o ./tmp/rfcptool_decode.o ./tmp/rfcptool_encode.o ./tmp/rfcptool_file.o ./tmp/rfcptool_main.o ./tmp/rfcptool_native.o ./tmp/rfcptool_util.o ./tmp/rfcptool_verify.o

./tmp/rfcptool_compile.o: ../../src/rfcptool_compile.c ../../src/rfcptool_def.h ../../src/rfcptool_util.h ../../src/rfcptool_file.h ../../src/rfcptool_compile.h
	mkdir -p ./tmp
	gcc -Wall -std=c90 -pedantic-errors -o ./tmp/rfcptool_compile.o -c ../../src/rfcptool_compile.c

./tmp/rfcptool_cpspec.o: ../../src/rfcptool_cpspec.c ../../src/rfcptool_def.h ../../src/rfcptool_util.h ../../src/rfcptool_file.h ../../src/rfcptool_compile.h ../../src/rfcptool_cpspec.h
	mkdir -p ./tmp
	gcc -Wall -std=c90 -pedantic-errors -o ./tmp/rfcptool_cpspec.o -c ../../src/rfcptool_cpspec.c

./tmp/rfcptool_decode.o: ../../src/rfcptool_decode.c ../../src/rfcptool_def.h ../../src/rfcptool_util.h ../../src/rfcptool_file.h ../../src/rfcptool_decode.h
	mkdir -p ./tmp
	gcc -Wall -std=c90 -pedantic-errors -o ./tmp/rfcptool_decode.o -c ../../src/rfcptool_decode.c

./tmp/rfcptool_encode.o: ../../src/rfcptool_encode.c ../../src/rfcptool_def.h ../../src/rfcptool_util.h ../../src/rfcptool_file.h ../../src/rfcptool_encode.h
	mkdir -p ./tmp
	gcc -Wall -std=c90 -pedantic-errors -o ./tmp/rfcptool_encode.o -c ../../src/rfcptool_encode.c

./tmp/rfcptool_file.o: ../../src/rfcptool_file.c ../../src/rfcptool_def.h ../../src/rfcptool_util.h ../../src/rfcptool_file.h
	mkdir -p ./tmp
	gcc -Wall -std=c90 -pedantic-errors -o ./tmp/rfcptool_file.o -c ../../src/rfcptool_file.c

./tmp/rfcptool_main.o: ../../src/rfcptool_main.c ../../src/rfcptool_def.h ../../src/rfcptool_util.h ../../src/rfcptool_decode.h ../../src/rfcptool_encode.h ../../src/rfcptool_cpspec.h ../../src/rfcptool_native.h ../../src/rfcptool_verify.h
	mkdir -p ./tmp
	gcc -Wall -std=c90 -pedantic-errors -o ./tmp/rfcptool_main.o -c ../../src/rfcptool_main.c

./tmp/rfcptool_native.o: ../../src/rfcptool_native.c ../../src/rfcptool_def.h ../../src/rfcptool_file.h ../../src/rfcptool_compile.h ../../src/rfcptool_native.h
	mkdir -p ./tmp
	gcc -Wall -std=c90 -pedantic-errors -o ./tmp/rfcptool_native.o -c ../../src/rfcptool_native.c

./tmp/rfcptool_util.o: ../../src/rfcptool_util.c ../../src/rfcptool_def.h ../../src/rfcptool_util.h
	mkdir -p ./tmp
	gcc -Wall -std=c90 -pedantic-errors -o ./tmp/rfcptool_util.o -c ../../src/rfcptool_util.c

./tmp/rfcptool_verify.o: ../../src/rfcptool_verify.c ../../src/rfcptool_def.h ../../src/rfcptool_util.h ../../src/rfcptool_verify.h
	mkdir -p ./tmp
	gcc -Wall -std=c90 -pedantic-errors -o ./tmp/rfcptool_verify.o -c ../../src/rfcptool_verify.c
