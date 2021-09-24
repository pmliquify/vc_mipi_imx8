#/bin/bash
#

BOARD=ixora
SOM=apalis

TARGET_IP=apalis-imx8
TARGET_USER=root

#DTB_FILE=imx8qm-apalis-v1.1-eval.dtb
DTO_FILE=apalis-imx8_vc_mipi_overlay

TEZI_RECOVER_URL=https://artifacts.toradex.com/artifactory/tezi-oe-prod-frankfurt/thud/release/4/apalis-imx8/tezi/tezi-run/oedeploy
TEZI_RECOVER=Apalis-iMX8_ToradexEasyInstaller_2.0b7-20210415
TEZI_URL=https://artifacts.toradex.com/artifactory/tdxref-oe-prod-frankfurt/dunfell-5.x.y/release/7/apalis-imx8/tdx-xwayland/tdx-reference-multimedia-image/oedeploy
TEZI=Apalis-iMX8_Reference-Multimedia-Image-Tezi_5.2.0+build.7
BOOTFS=Reference-Multimedia-Image-apalis-imx8.bootfs
ROOTFS=Reference-Multimedia-Image-apalis-imx8