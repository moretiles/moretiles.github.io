#!/bin/bash
set -e

sudo apt update
sudo apt install -y tmux screen pre-commit universal-ctags

rm -rf /var/lib/apt/lists/*
