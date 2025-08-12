#!/bin/bash
cd "$(dirname "$0")"
export SCRIPT_DIR=$(pwd)
export RECORDING_TIMESTAMP=$(date +%Y_%m_%d_%H_%M_%S)

# Create recording config
export IP_SELF=$(ip route get 8.8.8.8 | awk -F"src " 'NR==1{split($2,a," ");print a[1]}')

# Update recording config
cat $SCRIPT_DIR/rti_recording_config_template.xml \
| sed -e "s|TEMPLATE_IP|${IP_SELF}|g" \
| sed -e "s|TEMPLATE_RECORDING_FOLDER|${RECORDING_TIMESTAMP}|g" \
| sed -e "s|TEMPLATE_DOMAIN_ID|${DDS_DOMAIN}|g" \
| sed -e "s|TEMPLATE_NDDSHOME|${NDDSHOME}|g" \
>./rti_recording_config.xml


# Start recording
rtirecordingservice -cfgFile ./rti_recording_config.xml -cfgName cpm_recorder

# The script continues here after the user presses Ctrl+C