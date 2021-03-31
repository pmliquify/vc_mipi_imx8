# Alle Treiber auflisten
#cat /lib/modules/5.4.77-5.1.0+git.a2f08dfd79ae/modules.builtin | grep vc_mipi

# I2C Schnittstelle untersuchen
#i2cdetect -l
#i2cdetect -y 2  
#i2cget -a 2 0x10 0x01 b


# Standard DTB File in U-Boot für Apalis/Ixora
imx8qm-apalis-v1.1-eval.dtb
# Alternativen 
imx8qm-apalis-v1.1-ixora-v1.1.dtb


root@apalis-imx8:~# i2cdetect -l
i2c-3	i2c       	5a810000.i2c                    	I2C adapter
i2c-4	i2c       	5a820000.i2c                    	I2C adapter
i2c-2	i2c       	5a800000.i2c                    	I2C adapter
i2c-5	i2c       	5a830000.i2c                    	I2C adapter

root@apalis-imx8:~# i2cdetect -y 0
Error: Could not open file `/dev/i2c-0' or `/dev/i2c/0': No such file or directory

root@apalis-imx8:~# i2cdetect -y 1
Error: Could not open file `/dev/i2c-1' or `/dev/i2c/1': No such file or directory

root@apalis-imx8:~# i2cdetect -y 2
     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
00:          03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f 
10: 10 11 12 13 14 15 16 17 18 19 1a 1b 1c 1d 1e 1f 
20: 20 21 22 23 24 25 26 27 28 29 2a 2b 2c 2d 2e 2f 
30: -- -- -- -- -- -- -- -- 38 39 3a 3b 3c 3d 3e 3f 
40: 40 41 42 43 44 45 46 47 48 49 4a 4b 4c 4d 4e 4f 
50: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
60: 60 61 62 63 64 65 66 67 68 69 6a 6b 6c 6d 6e 6f 
70: 70 71 72 73 74 75 76 77  

root@apalis-imx8:~# i2cdetect -y 3
     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
00:          -- -- -- -- -- UU -- UU -- -- -- -- -- 
10: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
20: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
30: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
40: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
50: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
60: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
70: -- -- -- -- -- -- -- --                       