#! /bin/bash

SRC_FILE=${1}
BMP_FILE=${FILE_NAME%.*}.bmp
HOME_DIR=$(dirname $0)
TMP_DIR=${HOME_DIR}/tmp
DST_FILE=${2}

mkdir ${TMP_DIR} -p
convert +dither ${BMP_FILE} -map colors16.png -flip bmp3:${TMP_DIR}/${BMP_FILE}

${HOME_DIR}/bin/bmp2nibble ${TMP_DIR}/${BMP_FILE} ${DST_FILE}

