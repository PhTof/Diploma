SRC=./
BIN=./
PATCHELF=../dependencies/patchelf/src/patchelf
LIBPMEMDIR=../dependencies/pmdk
PMWATCHDIR=../dependencies/intel-pmwatch/pmwatch
LIBPMEMFLAGS=-isystem $(LIBPMEMDIR)/src/include -L $(LIBPMEMDIR)/src/debug -l:libpmem2.so
PMWATCHFLAGS=-L $(PMWATCHDIR)/lib64 -l:libpmwapi.so -l:libpmwcollect.so -I $(PMWATCHDIR)/include

all: simple hijack test nvmm

processes: $(SRC)/processes.c $(SRC)/numa.c $(SRC)/cpu.c $(SRC)/target.c $(SRC)/list.c $(SRC)/quickselect.c
	gcc -Wall -Werror -g $^ -o $@ -lnuma -lpthread # $(LIBPMEMFLAGS)
	# $(PATCHELF) --replace-needed libpmem2.so.1 $(LIBPMEMDIR)/src/debug/libpmem2.so.1 $@

cpu: $(SRC)/cpu.c
	gcc -o $(BIN)/cpu src/cpu.c -pthread

quickselect: $(SRC)/quickselect.c
	gcc $^ -o $(BIN)/$@

nvmm: $(SRC)/nvmm.c
	# Previosuly this used -c
	gcc -Werror -o $(BIN)/$@ $^ $(PMWATCHFLAGS)

simple:	$(SRC)/simple.c
	gcc $^ -o $(BIN)/$@

hijack: $(SRC)/hijack.c $(SRC)/dax.c $(SRC)/numa.c
	gcc -Wall -fPIC -shared -o $(BIN)/$@.so $^ -ldl $(LIBPMEMFLAGS)
	$(PATCHELF) --replace-needed libpmem2.so.1 $(LIBPMEMDIR)/src/debug/libpmem2.so.1 $(BIN)/$@.so

test: $(SRC)/test.c $(SRC)/dax.c $(SRC)/numa.c
	gcc -g -o $(BIN)/$@ $^ $(LIBPMEMFLAGS) -lnuma # -l pmem2
	$(PATCHELF) --replace-needed libpmem2.so.1 $(LIBPMEMDIR)/src/debug/libpmem2.so.1 $(BIN)/$@


clean:
	rm -f bin/* ./*.o
