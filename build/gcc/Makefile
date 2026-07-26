./bin/rfcptool: ./bin/rfcptool_compile.o ./bin/rfcptool_cpspec.o ./bin/rfcptool_decode.o ./bin/rfcptool_encode.o ./bin/rfcptool_file.o ./bin/rfcptool_main.o ./bin/rfcptool_native.o ./bin/rfcptool_util.o ./bin/rfcptool_verify.o
	mkdir -p ./bin
	gcc -o ./bin/rfcptool ./bin/rfcptool_compile.o ./bin/rfcptool_cpspec.o ./bin/rfcptool_decode.o ./bin/rfcptool_encode.o ./bin/rfcptool_file.o ./bin/rfcptool_main.o ./bin/rfcptool_native.o ./bin/rfcptool_util.o ./bin/rfcptool_verify.o

./bin/rfcptool_compile.o: ../../src/rfcptool_compile.c ../../src/rfcptool_def.h ../../src/rfcptool_util.h ../../src/rfcptool_file.h ../../src/rfcptool_compile.h
	mkdir -p ./bin
	gcc -Wall -std=c90 -pedantic-errors -o ./bin/rfcptool_compile.o -c ../../src/rfcptool_compile.c

./bin/rfcptool_cpspec.o: ../../src/rfcptool_cpspec.c ../../src/rfcptool_def.h ../../src/rfcptool_util.h ../../src/rfcptool_file.h ../../src/rfcptool_compile.h ../../src/rfcptool_cpspec.h
	mkdir -p ./bin
	gcc -Wall -std=c90 -pedantic-errors -o ./bin/rfcptool_cpspec.o -c ../../src/rfcptool_cpspec.c

./bin/rfcptool_decode.o: ../../src/rfcptool_decode.c ../../src/rfcptool_def.h ../../src/rfcptool_util.h ../../src/rfcptool_file.h ../../src/rfcptool_decode.h
	mkdir -p ./bin
	gcc -Wall -std=c90 -pedantic-errors -o ./bin/rfcptool_decode.o -c ../../src/rfcptool_decode.c

./bin/rfcptool_encode.o: ../../src/rfcptool_encode.c ../../src/rfcptool_def.h ../../src/rfcptool_util.h ../../src/rfcptool_file.h ../../src/rfcptool_encode.h
	mkdir -p ./bin
	gcc -Wall -std=c90 -pedantic-errors -o ./bin/rfcptool_encode.o -c ../../src/rfcptool_encode.c

./bin/rfcptool_file.o: ../../src/rfcptool_file.c ../../src/rfcptool_def.h ../../src/rfcptool_util.h ../../src/rfcptool_file.h
	mkdir -p ./bin
	gcc -Wall -std=c90 -pedantic-errors -o ./bin/rfcptool_file.o -c ../../src/rfcptool_file.c

./bin/rfcptool_main.o: ../../src/rfcptool_main.c ../../src/rfcptool_def.h ../../src/rfcptool_util.h ../../src/rfcptool_decode.h ../../src/rfcptool_encode.h ../../src/rfcptool_cpspec.h ../../src/rfcptool_native.h ../../src/rfcptool_verify.h
	mkdir -p ./bin
	gcc -Wall -std=c90 -pedantic-errors -o ./bin/rfcptool_main.o -c ../../src/rfcptool_main.c

./bin/rfcptool_native.o: ../../src/rfcptool_native.c ../../src/rfcptool_def.h ../../src/rfcptool_file.h ../../src/rfcptool_compile.h ../../src/rfcptool_native.h
	mkdir -p ./bin
	gcc -Wall -std=c90 -pedantic-errors -o ./bin/rfcptool_native.o -c ../../src/rfcptool_native.c

./bin/rfcptool_util.o: ../../src/rfcptool_util.c ../../src/rfcptool_def.h ../../src/rfcptool_util.h
	mkdir -p ./bin
	gcc -Wall -std=c90 -pedantic-errors -o ./bin/rfcptool_util.o -c ../../src/rfcptool_util.c

./bin/rfcptool_verify.o: ../../src/rfcptool_verify.c ../../src/rfcptool_def.h ../../src/rfcptool_util.h ../../src/rfcptool_verify.h
	mkdir -p ./bin
	gcc -Wall -std=c90 -pedantic-errors -o ./bin/rfcptool_verify.o -c ../../src/rfcptool_verify.c
