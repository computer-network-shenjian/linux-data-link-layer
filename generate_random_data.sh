#/bin/bash

# must run in bash
# this script generates random bytes from /dev/random and write to files of different size
# corresponding to the file names


# generate a 1 GB base file, ignoring dd's output
rand_onegig="dd if=/dev/random bs=1g count=1"

# commands for generate padding 
rand_one="dd if=/dev/urandom bs=1 count=1" # 1 byte
rand_ten="dd if=/dev/urandom bs=10 count=1" # 10 bytes
rand_onek="dd if=/dev/urandom bs=1024 count=1" # 1024 bytes

if [ $# -eq 0 ]; then
    echo "Usage: $0 size_in_mb"
else
    let "size = ($1 * 1024 * 1024) - 2" # a unicode char is 2 bytes long
    head -c $size </dev/urandom >rand_$1.myfile
    echo 1 >> rand_$1.myfile
fi
