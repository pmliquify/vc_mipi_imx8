FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://vc_mipi_camera.cfg"
SRC_URI += "file://vc_mipi_camera.c"
SRC_URI += "file://vc_mipi_core.c"
SRC_URI += "file://vc_mipi_core.h"
SRC_URI += "file://vc_mipi_modules.c"
SRC_URI += "file://vc_mipi_modules.h"
SRC_URI += "file://0001-Added-CIDs-for-trigger_mode-flash_mode-frame_rate-an.patch"
SRC_URI += "file://0001-Added-VC-MIPI-driver-files-to-.gitignore.patch"
SRC_URI += "file://0001-Bugfix-in-mipi_csi2_s_stream.-The-system-hung-on-str.patch"
SRC_URI += "file://0001-media-v4l-Add-Y14-format-support.patch"
SRC_URI += "file://0002-Added-VC-MIPI-driver-to-Kconfig-toradex_defconfig-an.patch"
SRC_URI += "file://0003-Added-pixelformat-GREY-Y10-Y12-Y14-RGGB-RG10-RG12-GB.patch"
SRC_URI += "file://0004-It-is-necessary-to-provide-the-driver-with-the-set-m.patch"

do_configure_append() {
        cp ${WORKDIR}/vc_mipi_camera.c ${S}/drivers/media/i2c
        cp ${WORKDIR}/vc_mipi_core.c ${S}/drivers/media/i2c
        cp ${WORKDIR}/vc_mipi_core.h ${S}/drivers/media/i2c
        cp ${WORKDIR}/vc_mipi_modules.c ${S}/drivers/media/i2c
        cp ${WORKDIR}/vc_mipi_modules.h ${S}/drivers/media/i2c
}