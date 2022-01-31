#!/usr/bin/env bash

BOARD=ixora
SOM=apalis_iMX8

TARGET_IP=apalis-imx8
TARGET_USER=root
DTO_FILE=apalis-imx8_vc_mipi_overlay

TEZI_RECOVER_URL=https://artifacts.toradex.com/artifactory/tezi-oe-prod-frankfurt/thud/release/4/apalis-imx8/tezi/tezi-run/oedeploy
TEZI_RECOVER=Apalis-iMX8_ToradexEasyInstaller_2.0b7-20210415
TEZI_URL=https://artifacts.toradex.com/artifactory/tdxref-oe-prod-frankfurt/dunfell-5.x.y/release/11/apalis-imx8/tdx-xwayland/tdx-reference-multimedia-image/oedeploy/
TEZI=Apalis-iMX8_Reference-Multimedia-Image-Tezi_5.4.0+build.11
BOOTFS=Reference-Multimedia-Image-apalis-imx8.bootfs
ROOTFS=Reference-Multimedia-Image-apalis-imx8