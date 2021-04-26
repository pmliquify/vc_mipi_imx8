#/bin/bash
#

SRC_DIR=$WORKING_DIR/src/apalis
TARGET_NAME=apalis-imx8
TARGET_SHELL="ssh root@$TARGET_NAME"
#DTB_FILE=imx8qm-apalis-v1.1-eval.dtb
DTO_FILE=apalis-imx8_vc_mipi_overlay

TEZI_URL=https://artifacts.toradex.com/artifactory/tdxref-oe-prod-frankfurt/dunfell-5.x.y/release/7/apalis-imx8/tdx-xwayland/tdx-reference-multimedia-image/oedeploy
TEZI=Apalis-iMX8_Reference-Multimedia-Image-Tezi_5.2.0+build.7

BOOTFS=Reference-Multimedia-Image-apalis-imx8.bootfs