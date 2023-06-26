# sudo apt-get install autoconf pkg-config libndctl-dev libdaxctl-dev pandoc

INIT_PATH=$(pwd)
MYPATH=$(readlink -f ../dependencies)

function replace {
        # escape whatever needed
        printf -v sed_command 's|%q|%q|g' "$1" "$2"
        # we don't want to escape the "single quote" character
        sed_command=$(echo $sed_command | sed -e "s/\\\'/'/g")
	# run sed
	sed -i -r "$sed_command" $3 
}

mkdir -p $MYPATH

cd $MYPATH

git clone https://github.com/intel/intel-pmwatch.git
cd intel-pmwatch
./autogen.sh
PMWATCHDIR=$(pwd)/pmwatch
mkdir $PMWATCHDIR
./configure --prefix=$PMWATCHDIR --bindir=$PMWATCHDIR/bin64 \
	--libdir=$PMWATCHDIR/lib64
make -j `proc`
make -j install
cd ../

# get a newer meson version (used by ndctl)
# otherwise we recieve "Unknown type feature" ERROR
wget https://github.com/mesonbuild/meson/archive/refs/tags/0.60.0.zip
unzip -q 0.60.0.zip
mv meson-0.60.0 meson
rm 0.60.0.zip

# Get iniparser (needed by ndctl)
git clone https://salsa.debian.org/carnil/iniparser.git
cd iniparser
make
cd ../

# Get keyutils (needed by ndctl)
git clone http://git.kernel.org/pub/scm/linux/kernel/git/dhowells/keyutils.git
cd keyutils
# An unused function causes problems with -Werror
sed -i -r 's/-Werror//g' Makefile
make
cd ../

# Get json-c (needed by ndctl)
git clone https://salsa.debian.org/debian/json-c.git
cd json-c
cmake .
make -j `nproc`
mkdir json-c
cp json.h printbuf.h json-c
cd ../

# Make ndctl (required by pmdk)
git clone https://github.com/pmem/ndctl.git
cd ndctl

# Fix the dependency issues
OLDLINE="json = dependency('json-c')"
NEWLINE="json = cc.find_library('json-c', dirs : '$MYPATH/json-c')"
replace "$OLDLINE" "$NEWLINE" meson.build

OLDLINE="keyutils = cc.find_library('keyutils', required : get_option('keyutils'))"
NEWLINE="keyutils = cc.find_library('keyutils', required : get_option('keyutils'), dirs : '$MYPATH/keyutils')"
replace "$OLDLINE" "$NEWLINE" meson.build

LIBRARY_PATH=$MYPATH/keyutils:$MYPATH/iniparser \
C_INCLUDE_PATH=$MYPATH/keyutils:$MYPATH/iniparser/src:$MYPATH/json-c \
python3 ../meson/meson.py setup build

LIBRARY_PATH=$MYPATH/keyutils:$MYPATH/iniparser \
C_INCLUDE_PATH=$MYPATH/keyutils:$MYPATH/iniparser/src:$MYPATH/json-c \
python3 ../meson/meson.py compile -C build

cd ../

# Get patchelf
git clone https://github.com/NixOS/patchelf
cd patchelf
./bootstrap.sh
./configure
make
make check
cd ../

PATCHELF=$MYPATH/patchelf/src/patchelf

# Make PMDK
git clone https://github.com/pmem/pmdk
cd pmdk
git checkout tags/1.12.0

NDCTLBUILD="$MYPATH/ndctl/build"
OLDLINE="LIBNDCTL_LIBS := \$(shell \$(PKG_CONFIG) --libs \$(LIBNDCTL_PKG_CONFIG_DEPS))"
NEWLINE="LIBNDCTL_LIBS=-L $NDCTLBUILD/ndctl/lib/ -l:libndctl.so -L $NDCTLBUILD/daxctl/lib -l:libdaxctl.so"
replace "$OLDLINE" "$NEWLINE" src/common.inc

NEWLINE="\$(LINKER) -o \$@ $^ \$(LDFLAGS) \$(LIBS)"
OLDLINE="LD_LIBRARY_PATH=\$(LIBFABRIC_LD_LIBRARY_PATHS):\$(LIBNDCTL_LD_LIBRARY_PATHS):\$(LD_LIBRARY_PATH) $NEWLINE"
replace "$OLDLINE" "$NEWLINE" src/examples/Makefile.inc

LD_LIBRARY_PATH=$NDCTLBUILD/ndctl/lib:$NDCTLBUILD/daxctl/lib \
LIBRARY_PATH=$NDCTLBUILD/ndctl/lib:$NDCTLBUILD/daxctl/lib \
C_INCLUDE_PATH=$MYPATH/ndctl \
PKG_CONFIG_PATH=$NDCTLBUILD/ndctl/lib:$NDCTLBUILD/daxctl/lib \
make -j `nproc`

# IMPORTANT: when linking anything with libpmem*, we should
# export a proper value of LD_LIBRARY_PATH (in this case, that
# is LD_LIBRARY_PATH=$NDCTLBUILD/ndctl/lib:$NDCTLBUILD/daxctl/lib)
# in order for the linking to occur properly.
# To observe this, try ldd /path/to/libpmem2.so with and without
# updating LD_LIBRARY_PATH

# Another solution is this:

# Patch the required DLIB dependencies
$PATCHELF --replace-needed libndctl.so.6 $NDCTLBUILD/ndctl/lib/libndctl.so.6 ./src/debug/libpmem2.so
$PATCHELF --replace-needed libdaxctl.so.1 $NDCTLBUILD/daxctl/lib/libdaxctl.so.1 ./src/debug/libpmem2.so

cd $INIT_PATH
