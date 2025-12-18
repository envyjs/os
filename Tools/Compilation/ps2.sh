#!/bin/bash
# Envy WebUI compilation script for PS2
echo "Building Envy ISO for PS2"
mkdir ./Output/iso/ps2/
echo "Building Envy ISO for PS2 Phat (FW 2.10-2.13)"
genisoimage -udf -o ./Output/iso/ps2/envy-oldphat.iso "./Platforms/ps2/dvdfs/phat/2.10"
echo "Building Envy ISO for PS2 Phat (FW 3.04M and above)"
genisoimage -udf -o ./Output/iso/ps2/envy-newphat.iso "./Platforms/ps2/dvdfs/phat/3.04M"
echo "Building Envy ISO for PS2 Slim"
genisoimage -udf -o ./Output/iso/ps2/envy-slim.iso "./Platforms/ps2/dvdfs/slim"