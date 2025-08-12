# Put this into external file / find out if it is really necessary
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib64/:/tmp/software/cpm_lib/build/:/opt/rti_connext_dds-6.0.0/lib/x64Linux4gcc7.3.0
export PATH=$PATH:$HOME/bin:$HOME/.local/bin:/opt/rti_connext_dds-6.0.0:/opt/rti_connext_dds-6.0.0/bin:/opt/rti_connext_dds-6.0.0/lib/x64Linux4gcc7.3.0:/opt/rti_connext_dds-6.0.0/bin:/opt/raspbian-toolchain-gcc-4.7.2-linux64/bin
export NDDSHOME=/opt/rti_connext_dds-6.0.0
export RASPBIAN_TOOLCHAIN=/opt/raspbian-toolchain-gcc-4.7.2-linux64
export RTI_LICENSE_FILE=/opt/rti_connext_dds-6.0.0/rti_license.dat

# Replace this if you want the software to run on a PC that is not the Lab's main PC
export IP_SELF=$(ip route get 8.8.8.8 | awk -F"src " 'NR==1{split($2,a," ");print a[1]}')
export DDS_INITIAL_PEER=rtps@udpv4://$IP_SELF:25598
