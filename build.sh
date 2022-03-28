CC=clang
if [ -d "./bin/generated" ]; then rm -rf "./bin/generated"; echo "[INFO] Removing ./bin/generated/"; fi
if [ -d "./generated" ]; then rm -rf "./generated"; echo "[INFO] Removing ./generated"; fi

CFILES="./source/*.c"
CFLAGS="-Wall -Wvarargs -Werror -Wno-unused-function -Wno-format-security -Wno-incompatible-pointer-types-discards-qualifiers `llvm-config --cflags`"
IFLAGS="-Isource -Ithird-party/include -I/usr/local/include"
LDFLAGS="`llvm-config --ldflags`"
LIBS="`llvm-config --libs` `llvm-config --system-libs` -lstdc++ -lm"
DEFINES="-D_DEBUG -D_CRT_SECURE_NO_WARNINGS"
OUT="-o bin/rift"

mkdir -p bin/
echo "[INFO] Creating ./bin"
echo "[INFO] Building Rift executable"

${CC} ${CFILES} ${CFLAGS} ${DEFINES} ${IFLAGS} ${LDFLAGS} ${LIBS} ${OUT} 
