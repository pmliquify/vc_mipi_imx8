# Alle Treiber auflisten
#cat /lib/modules/5.4.77-5.1.0+git.a2f08dfd79ae/modules.builtin | grep vc_mipi

# I2C Schnittstelle untersuchen
#i2cdetect -l
#i2cdetect -y 2  
#i2cget -a 2 0x10 0x01 b


# Standard DTB File in U-Boot für Apalis/Ixora
imx8qm-apalis-v1.1-eval.dtb
# Alternativen
imx8qm-apalis-ixora-v1.1.dtb
imx8qm-apalis-v1.1-ixora-v1.1.dtb