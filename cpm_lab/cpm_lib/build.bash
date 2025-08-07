#!/bin/bash
# exit when any command fails
set -e

# DIR holds the location of build.bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

cd $DIR

rm -rf $DIR/cpm_library_package
mkdir $DIR/cpm_library_package

if [ ! -d "dds_idl_cpp" ]; then
    echo "Generating C++ IDL files..."
    ./rtigen.bash
fi

if [[ ! -z $(which matlab) ]] && [[ ! -d "dds_idl_matlab" ]]; then
	echo "Generating Matlab IDL files..."
    matlab -sd "./" -batch "rtigen_matlab"
fi 

if [[ -d "dds_idl_matlab" ]]; then
    cp -R $DIR/dds_idl_matlab/ $DIR/cpm_library_package
fi 

mkdir -p $DIR/build

# Copy local communication QoS, use correct IP
IP_SELF=$(ip route get 8.8.8.8 | awk -F"src " 'NR==1{split($2,a," ");print a[1]}')
sed -e "s/TEMPLATE_IP/${IP_SELF}/g" \
<$DIR/QOS_LOCAL_COMMUNICATION.xml.template \
>$DIR/build/QOS_LOCAL_COMMUNICATION.xml

cd $DIR/build
cmake ..
make -C $DIR/build -j$(nproc) && $DIR/build/unittest
cd $DIR

# Publish cpm_library package via http/apache for the HLCs to download
if [ -z $SIMULATION ]; then
    if [ ! -d "/var/www/html/nuc" ]; then
        sudo mkdir -p "/var/www/html/nuc"
        sudo chmod a+rwx "/var/www/html/nuc"
    fi
    cp $DIR/build/libcpm.so $DIR/cpm_library_package
    cp ${DIR}/QOS_LOCAL_COMMUNICATION.xml.template ./cpm_library_package
    tar -czf cpm_library_package.tar.gz -C $DIR/ cpm_library_package
    rm -f /var/www/html/nuc/cpm_library_package.tar.gz
    mv $DIR/cpm_library_package.tar.gz /var/www/html/nuc
fi
