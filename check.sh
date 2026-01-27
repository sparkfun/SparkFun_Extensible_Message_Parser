#!/bin/bash
#
# check.sh
#    Script to verify example builds
########################################################################
set -e
#set -o verbose
#set -o xtrace

# Base_Test
cd examples/Base_Test
make clean
make
make clean

# Mixed_Parser
cd ../Mixed_Parser
make clean
make
make clean

# Multiple_Parsers
cd ../Multiple_Parsers
make clean
make
make clean

# NMEA_Test
cd ../NMEA_Test
make clean
make
make clean

# RTCM_Test
cd ../RTCM_Test
make clean
make
make clean

# SBF_in_SPARTN_Test
cd ../SBF_in_SPARTN_Test
make clean
make
make clean

# SBF_Test
cd ../SBF_Test
make clean
make
make clean

# SPARTN_Test
cd ../SPARTN_Test
make clean
make
make clean

# UBLOX_Test
cd ../UBLOX_Test
make clean
make
make clean

# Unicore_Binary_Test
cd ../Unicore_Binary_Test
make clean
make
make clean

# Unicore_Hash_Test
cd ../Unicore_Hash_Test
make clean
make
make clean

# User_Parser_Test
cd ../User_Parser_Test
make clean
make
make clean

# Return to origin directory
cd ../..
