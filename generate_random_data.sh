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


# concatenate with a non-zero byte in the end to avoid a \0 ending
echo $($rand_onegig 2>/dev/null) $($rand_one 2>/dev/null) 1 1>rand_1g1byte 
echo $($rand_onegig 2>/dev/null) $($rand_ten 2>/dev/null) 1 1>rand_1g10byte 
echo $($rand_onegig 2>/dev/null) $($rand_onek 2>/dev/null) 1 1>rand_1g1m 
