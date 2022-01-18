# Package summary
SUMMARY = "VC MIPI Demo"
# License, for example MIT
LICENSE = "MIT"
# License checksum file is always required
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"
 
# Sources from GitHub
SRCREV = "8fabe255fc36294ad536c5a625bc2ce474b35c1f"
SRC_URI += "git://github.com/pmliquify/vc_mipi_demo.git;protocol=https;branch=develop"
SRC_URI[sha256sum] = "cd0a67e778da5c42f50ad65021d158b94fcf53e368469a14fd5e92564ecdb5e5"
 
# Set LDFLAGS options provided by the build system
TARGET_CC_ARCH += "${LDFLAGS}"
 
# Change source directory to workdirectory
S = "${WORKDIR}/git"
 
# Compile vcmipidemo from sources, no Makefile
do_compile() {
    oe_runmake -C src vcmipidemo
}
 
# Install binary to final directory /usr/bin
do_install() {
    install -d ${D}${bindir}
    install -m 0755 ${S}/src/vcmipidemo ${D}${bindir}
}