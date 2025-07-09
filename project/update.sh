#!/bin/bash

# Define source and destination directories
HOME=~
mbedTLS_VERSON="3.1.1"
mbedTLS_source="$HOME/STM32Cube/Repository/Packs/ARM/mbedTLS/$mbedTLS_VERSON/library/"
mbedTLS_destination="./Middlewares/Third_Party/ARM_Security/"

LwIP_destination="./Middlewares/Third_Party/lwIP_Network_lwIP/"

# Create the destination directory if it doesn't exist
if [ ! -d "$mbedTLS_destination" ]; then
    mkdir -p "$mbedTLS_destination"
fi

echo "Home : " $HOME

# Copy the contents from mbedTLS_source to mbedTLS_destination
cp -r "$mbedTLS_source" "$mbedTLS_destination"
echo "Content copied from $mbedTLS_source to $mbedTLS_destination"

FILE_PATH=$mbedTLS_destination"include/mbedtls/mbedtls_config.h"
echo "Deleting $FILE_PATH"
rm -f $FILE_PATH

FILE_PATH=$LwIP_destination"ports/freertos/include"
echo "Deleting $FILE_PATH"
rm -r -f $FILE_PATH
