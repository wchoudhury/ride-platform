#! /bin/bash

# Check if conda is installed
if ! command -v conda &> /dev/null
then
    echo "Conda could not be found"

    # install miniconda
    mkdir -p ~/miniconda3
    wget https://repo.anaconda.com/miniconda/Miniconda3-latest-Linux-x86_64.sh -O ~/miniconda3/miniconda.sh
    bash ~/miniconda3/miniconda.sh -b -u -p ~/miniconda3
    rm ~/miniconda3/miniconda.sh

    # add conda to path
    export PATH=~/miniconda3/bin:$PATH

    # add to bashrc
    echo 'export PATH=~/miniconda3/bin:$PATH' >> ~/.bashrc

    # source bashrc
    source ~/.bashrc

    exit
fi

# check if donkey conda environment exists
if ! conda env list | grep -q "donkey"
then
    conda create -n donkey python=3.11
fi

# activate donkey conda environment
source activate base
conda activate donkey

cd donkeycar
pip install -e .[pc]

cd ..

# install gym donkey
cd gym-donkeycar
pip install -e .[gym-donkeycar]

