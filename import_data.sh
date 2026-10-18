#!/bin/bash

function print_missing_data() {
    echo "[WARNING] It seems this script is not run from the dev container."
    echo "Data import may not be complete."
    echo "Please scp the data from the dev container to ./data/ directory on"
    echo "your local environment if this is not the dev container."
}

BASEDIR="$(cd "`dirname "$0"`" && pwd)"
cd "$BASEDIR"

if [ ! -d ./.git ]; then
    echo "Error: import_data.sh must be run from your local repository root"
    echo ""
    echo "Hint: did you extracted the supplemental file tarball at your local repository"
    echo "root? If so, 'ls -al' should print a line for the .git directory."
    exit 1
fi

LOCAL_DATA_DIR_PREFIX=/ro-data/testdata

# create data directory on local disk if this is a student server
mkdir -p data
cd data

if [ ! -d "$LOCAL_DATA_DIR_PREFIX" ]; then
    print_missing_data
    exit 0
fi

if [ ! -d ./preloaded_btree ]; then
    echo "Extracting preloaded_btree.tar.xz"
    tar xf "${LOCAL_DATA_DIR_PREFIX}/preloaded_btree.tar.xz"
else
    echo "Skipping preloaded_btree -- already exists"
fi

if [ ! -d ./tpch_s01_ans ]; then
    echo "Extracting tpch_s01_ans.tar.xz"
    tar xf "${LOCAL_DATA_DIR_PREFIX}/tpch_s01_ans.tar.xz"
else
    echo "Skipping tpch_s01_ans -- already exists"
fi

if [ ! -d ./tpch_s01 ]; then
    echo "Extracting tpch_s01.tar.xz"
    tar xf "${LOCAL_DATA_DIR_PREFIX}/tpch_s01.tar.xz"
else
    echo "Skipping tpch_s01 -- already exists"
fi

if [ ! -d ./tpch_s1_ans ]; then
    echo "Extracting tpch_s1_ans.tar.xz"
    tar xf "${LOCAL_DATA_DIR_PREFIX}/tpch_s1_ans.tar.xz"
else
    echo "Skipping tpch_s1_ans -- already exists"
fi

if [ ! -d ./tpch_s1 ]; then
    echo "Extracting tpch_s1.tar.xz"
    tar xf "${LOCAL_DATA_DIR_PREFIX}/tpch_s1.tar.xz"
else
    echo "Skipping tpch_s1 -- already exists"
fi

