#!/bin/sh
# -*- coding: utf-8 -*-
#
#ツールチェイン構築スクリプト
#
#x86-64 Linux環境で, 本スクリプトとおなじディレクトリに 
# - yatos-newlib-2_4_0.patch 
# - gdb-7.10-qemu-x86-64.patch
#を配置して,
#
#/bin/sh ./cross.sh
#
#を実行すると, 
#${HOME}/cross/yatos/構築日
#配下にx86_64-elf-のツールチェインを構築・インストールする
#
#${HOME}/cross/yatos/current/bin
#にパスを通しておくことで直近にビルドしたツールチェインを利用することができます。
#
DATE=`date +%F`
TARGET_CPU=x86_64
LANG=C

if [ "x${NR_CPUS}" != "x" ]; then
    export SMP_FLG="-j${NR_CPUS}"
else
    NR_CPUS=`cat /proc/cpuinfo|grep processor|wc -l`
    export SMP_FLG="-j${NR_CPUS}"
fi
export TOP_DIR=`pwd`

export PREFIX="${HOME}/cross/yatos/${DATE}"
export TARGET=${TARGET_CPU}-elf
export PATH="${HOME}/cross/autotools/bin:${PREFIX}/bin:$PATH"
export SYSROOT="${PREFIX}/rfs"
export ARCHIVE_DIR=`pwd`/archives
export SRC_DIR=`pwd`/src
export BUILD_DIR=`pwd`/build
export PATH="${PREFIX}/bin:${PATH}"

AUTOCONF_URL=http://ftp.gnu.org/gnu/autoconf/autoconf-2.64.tar.gz
AUTOMAKE_URL=http://ftp.gnu.org/gnu/automake/automake-1.12.6.tar.gz
BINUTILS_URL=http://ftp.gnu.org/gnu/binutils/binutils-2.24.tar.gz
GCC_URL=http://ftp.gnu.org/gnu/gcc/gcc-4.8.3/gcc-4.8.3.tar.gz
NEWLIB_URL=ftp://sourceware.org/pub/newlib/newlib-2.4.0.tar.gz
GDB_URL=http://ftp.gnu.org/gnu/gdb/gdb-7.10.tar.gz

NEWLIB_PATCH=${TOP_DIR}/yatos-newlib-2_4_0.patch
GDB_PATCH=${TOP_DIR}/gdb-7.10-qemu-x86-64.patch

prepare(){

    rm -fr "${PREFIX}" "${SRC_DIR}" "${BUILD_DIR}"

    mkdir -p "${PREFIX}"
    mkdir -p "${SYSROOT}"
    mkdir -p "${ARCHIVE_DIR}"
    mkdir -p "${SRC_DIR}"
    mkdir -p "${BUILD_DIR}"

    pushd "${PREFIX}"
    cd ..
    ln -sf ${DATE} current
    popd
}

fetch_archives(){

    rm -fr "${ARCHIVE_DIR}"
    mkdir -p "${ARCHIVE_DIR}"
    pushd  "${ARCHIVE_DIR}"
    wget ${AUTOMAKE_URL}
    wget ${AUTOCONF_URL}
    wget ${BINUTILS_URL}
    wget ${GCC_URL}
    wget ${NEWLIB_URL}
    wget ${GDB_URL}
    popd

}

build_autotools(){
    local automake=`basename ${AUTOMAKE_URL}`
    local autoconf=`basename ${AUTOCONF_URL}`
    local am_base=`basename ${automake} .tar.gz`
    local ac_base=`basename ${automake} .tar.gz`

    pushd ${SRC_DIR}
    tar xf ${ARCHIVE_DIR}/${automake}
    tar xf ${ARCHIVE_DIR}/${autoconf}    
    popd

    pushd ${BUILD_DIR}

    mkdir autoconf
    pushd autoconf
    ${SRC_DIR}/${ac_base}/configure --prefix=${PREFIX}
    make
    make install
    popd

    mkdir automake
    pushd automake
    ${SRC_DIR}/${am_base}/configure --prefix=${PREFIX}
    make
    make install
    popd

    popd
}

build_binutils(){
    local binutils=`basename ${BINUTILS_URL}`
    local binutils_base=`basename ${binutils} .tar.gz`

    pushd ${SRC_DIR}
    tar xf ${ARCHIVE_DIR}/${binutils}
    popd

    mkdir -p ${BUILD_DIR}/binutils
    pushd ${BUILD_DIR}/binutils
    ${SRC_DIR}/${binutils_base}/configure --prefix="${PREFIX}" \
     --target=${TARGET} --with-sysroot="${SYSROOT}" --disable-nls --disable-werror
    make ${SMP_FLG} 
    make install
    popd
}

build_gcc1(){
    local gcc=`basename ${GCC_URL}`
    local gcc_base=`basename ${gcc} .tar.gz`

    pushd ${SRC_DIR}
    tar xf ${ARCHIVE_DIR}/${gcc}
    popd

    mkdir -p ${BUILD_DIR}/gcc1
    pushd ${BUILD_DIR}/gcc1
    ${SRC_DIR}/${gcc_base}/configure --target=${TARGET} --prefix="${PREFIX}" \
	--disable-nls --enable-languages=c,c++ --without-headers
    make ${SMP_FLG} all-gcc
    make ${SMP_FLG} all-target-libgcc
    make install-gcc
    make install-target-libgcc
    popd
}

build_newlib(){
    local newlib=`basename ${NEWLIB_URL}`
    local newlib_base=`basename ${newlib} .tar.gz`

    pushd ${SRC_DIR}
    rm -fr "${SRC_DIR}/${newlib_base}"
    tar xf ${ARCHIVE_DIR}/${newlib}
    pushd ${SRC_DIR}/${newlib_base}

    patch -p1 < ${NEWLIB_PATCH}

    pushd newlib/libc/sys
    autoconf

    pushd yatos
    autoreconf
    popd

    popd

    popd

    popd
    mkdir -p ${BUILD_DIR}/newlib
    pushd ${BUILD_DIR}/newlib
    ${SRC_DIR}/${newlib_base}/newlib/configure --target=${TARGET_CPU}-pc-yatos \
	--host=${TARGET_CPU}-pc-yatos --prefix=/usr --disable-multilib
    sed -i 's/TARGET=${TARGET_CPU}-pc-yatos-/TARGET=/g' Makefile
    sed -i 's/WRAPPER) ${TARGET_CPU}-pc-yatos-/WRAPPER) /g' Makefile
    make
    make DESTDIR="${SYSROOT}" install

    pushd ${SYSROOT}/usr
    mv ${TARGET_CPU}-pc-yatos/* .
    rm -fr "${TARGET_CPU}-pc-yatos"
    popd
}

build_gcc2(){
    local gcc=`basename ${GCC_URL}`
    local gcc_base=`basename ${gcc} .tar.gz`

    pushd ${SRC_DIR}
    tar xf ${ARCHIVE_DIR}/${gcc}
    popd

    mkdir -p ${BUILD_DIR}/gcc2
    pushd ${BUILD_DIR}/gcc2
    ${SRC_DIR}/${gcc_base}/configure --target=${TARGET} --prefix="${PREFIX}" \
	--disable-nls --enable-languages=c,c++ --with-sysroot="${SYSROOT}"

    make ${SMP_FLG} all-gcc
    make ${SMP_FLG} all-target-libgcc
    make install-gcc
    make install-target-libgcc
    popd
}

build_gdb(){
    local gdb=`basename ${GDB_URL}`
    local gdb_base=`basename ${gdb} .tar.gz`

    pushd ${SRC_DIR}
    tar xf ${ARCHIVE_DIR}/${gdb}
    popd

    pushd ${SRC_DIR}/${gdb_base}
    patch -p1 < ${GDB_PATCH}
    popd

    mkdir -p ${BUILD_DIR}/gdb
    pushd ${BUILD_DIR}/gdb
    #
    #gdb は, x86_64-*-elfターゲットをサポートしていないので, セルフ環境のgdbを作成
    #
    ${SRC_DIR}/${gdb_base}/configure --program-prefix="${TARGET}-" --prefix="${PREFIX}" \
	--disable-nls
    make
    make install
    popd
}

prepare
fetch_archives
build_autotools
build_binutils
build_gcc1
build_newlib
build_gcc2
build_gdb

# mkdir build-gcc1
# pushd build-gcc1
# ../../src/gcc-4.8.3/configure --target=${TARGET} --prefix="${PREFIX}" \
#  --disable-nls --enable-languages=c,c++ --without-headers
# make ${SMP_FLG} all-gcc
# make ${SMP_FLG} all-target-libgcc
# make install-gcc
# make install-target-libgcc
# popd

# mkdir build-newlib
# pushd build-newlib
# ../../src/newlib-2.4.0/newlib/configure --target=${TARGET_CPU}-pc-yatos --host=${TARGET_CPU}-pc-yatos \
#     --prefix=/usr --disable-multilib
# sed -i 's/TARGET=${TARGET_CPU}-pc-yatos-/TARGET=/g' Makefile
# sed -i 's/WRAPPER) ${TARGET_CPU}-pc-yatos-/WRAPPER) /g' Makefile
# make ${SMP_FLG} 
# make DESTDIR="${SYSROOT}" install
# ln -sf ${SYSROOT}/usr/${TARGET_CPU}-pc-yatos/include ${SYSROOT}/usr
# ln -sf ${SYSROOT}/usr/${TARGET_CPU}-pc-yatos/lib ${SYSROOT}/usr
# popd

# mkdir build-gcc2
# pushd build-gcc2
# ../../src/gcc-4.8.3/configure --target=${TARGET} --prefix="${PREFIX}" \
#  --disable-nls --enable-languages=c,c++ --with-sysroot="${SYSROOT}"
# make ${SMP_FLG} all-gcc
# make ${SMP_FLG} all-target-libgcc
# make install-gcc
# make install-target-libgcc
# popd
