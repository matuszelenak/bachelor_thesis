#!/bin/bash
clear; clear; g++ HMM.cpp State.cpp LogNum.cpp main.cpp --std=c++14 -O2 -o hmm_sampler -lboost_program_options -lpthread -lboost_log -DBOOST_LOG_DYN_LINK -I /usr/include/hdf5/serial/ -lhdf5_serial