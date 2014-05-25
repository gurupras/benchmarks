#!/bin/bash

# Simple script to generate Makefile from .Makefile
# and user provided path for the shared tuning library


LIB_NAME=libpatuning
LIB_PATH=

set_path() {
	echo -n "Enter path to tuning library:"
	read -e LIB_PATH
	CWD=`pwd`
	cd $LIB_PATH && TMP=`ls $LIB_NAME.so $LIB_NAME.a`
	cd $CWD
	if [[ -z "$TMP" ]]; then
		echo "Warning: " $LIB_NAME ".so and " $LIB_NAME ".a not found!"
	fi
	TMP=`tempfile`
	echo $TMP
	cp .Makefile $TMP
	sed -i "s|TUNING_LIB_PATH=.*|TUNING_LIB_PATH=$LIB_PATH|g" $TMP
	cp $TMP ./Makefile

	echo "Makefile generated."
}

replace_path() {
	echo "Existing path:" $EXISTING_PATH
	set_path
}

# Is there already an existing path?
if [[ ! -f ./Makefile ]]; then
	set_path
else
	EXISTING_PATH=$(cat Makefile | grep "TUNING_LIB_PATH=.*" | sed 's/TUNING_LIB_PATH=//g')
	if [[ -z "$EXISTING_PATH" ]]; then
		set_path
	else
		replace_path
	fi
fi

