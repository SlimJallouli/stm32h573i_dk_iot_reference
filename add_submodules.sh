#!/bin/bash

git submodule add https://github.com/littlefs-project/littlefs.git project/Libraries/littlefs
git submodule add https://github.com/FreeRTOS/corePKCS11.git project/Libraries/corePKCS11
git submodule add https://github.com/intel/tinycbor.git project/Libraries/tinycbor

git submodule update --init --recursive
git submodule status
