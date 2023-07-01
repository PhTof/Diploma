SRC=./
BIN=./
PATCHELF=../dependencies/patchelf/src/patchelf
LIBPMEMDIR=../dependencies/pmdk
PMWATCHDIR=../dependencies/intel-pmwatch/pmwatch_bin
LIBPMEMFLAGS=-isystem $(LIBPMEMDIR)/src/include -L $(LIBPMEMDIR)/src/debug -l:libpmem2.so
PMWATCHFLAGS=-L $(PMWATCHDIR)/lib64 -l:libpmwapi.so -l:libpmwcollect.so -I $(PMWATCHDIR)/include

all: simple hijack test nvmm

processes: processes.o nvmm.o numa.o cpu.o target.o list.o quickselect.o
	# gcc -Wall -Werror -g $^ -o $@ -lnuma -lpthread $(PMWATCHFLAGS) # $(LIBPMEMFLAGS)
	gcc -g $^ -o $@ -lnuma -lpthread $(PMWATCHFLAGS) # $(LIBPMEMFLAGS)
	# $(PATCHELF) --replace-needed libpmem2.so.1 $(LIBPMEMDIR)/src/debug/libpmem2.so.1 $@

%.o: $(SRC)/%.c
	gcc -c $^

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
	rm -f hijack.so processes bin/* ./*.o
