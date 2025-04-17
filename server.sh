#!/bin/bash
git pull

g++ src/server/main.cpp -o server
./server

rm server