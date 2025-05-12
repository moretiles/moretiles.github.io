#!/bin/bash
set -e

sudo apt update
sudo apt install -y golang

rm -rf /var/lib/apt/lists/*
