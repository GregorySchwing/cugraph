#!/usr/bin/env bash
# Copyright (c) 2019-2023, NVIDIA CORPORATION.

# This assumes the script is executed from the root of the repo directory

# NOTE: the "libcugraph" target also builds all single-gpu gtest binaries, and
# the "cpp-mgtests" target builds the multi-gpu gtest binaries (and requires the
# openmpi build dependencies). The conda package does NOT include these test
# binaries or extra dependencies, but these are built here for use in CI runs.

+export NVCC_PREPEND_FLAGS="${NVCC_PREPEND_FLAGS} -ccbin ${CXX}" # Needed for CUDA 12 nvidia channel compilers
./build.sh libcugraph libcugraph_etl cpp-mgtests -n -v --allgpuarch
