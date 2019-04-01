#
# Component Makefile
#

COMPONENT_ADD_INCLUDEDIRS := \
	qcloud-iot-sdk-embedded-c/src/sdk-impl \
	qcloud-iot-sdk-embedded-c/src/sdk-impl/exports \
	qcloud-iot-sdk-embedded-c/src/mqtt/include \
	qcloud-iot-sdk-embedded-c/src/device/include \
	qcloud-iot-sdk-embedded-c/src/system/include \
	qcloud-iot-sdk-embedded-c/src/utils/digest \
	qcloud-iot-sdk-embedded-c/src/utils/farra \
	qcloud-iot-sdk-embedded-c/src/utils/lite \
	port/include 

COMPONENT_SRCDIRS := \
	qcloud-iot-sdk-embedded-c/src/sdk-impl \
	qcloud-iot-sdk-embedded-c/src/platform/os/linux \
	qcloud-iot-sdk-embedded-c/src/mqtt/src \
	qcloud-iot-sdk-embedded-c/src/utils/digest \
	qcloud-iot-sdk-embedded-c/src/utils/farra \
	qcloud-iot-sdk-embedded-c/src/utils/lite \
	qcloud-iot-sdk-embedded-c/src/device/src 
	
