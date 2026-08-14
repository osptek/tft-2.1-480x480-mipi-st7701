# 1. 准备工作

```
sudo apt update
sudo apt install raspberrypi-kernel-headers build-essential device-tree-compiler
mkdir st7701-480x480 && cd st7701-480x480
```

# 2. 驱动源码（panel-st7701-480x480.c）

```
sudo nano panel-st7701-480x480.c
```



# 3. Makefile

```
sudo nano Makefile
```



```
obj-m += panel-st7701-480x480.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
```

> 编译：

```
make clean
make
```

# 4. 设备树 Overlay（vc4-kms-dsi-st7701-480x480.dts）

```
sudo nano vc4-kms-dsi-st7701-480x480.dts
```



> 编译并安装：

```
dtc -@ -I dts -O dtb -o vc4-kms-dsi-st7701-480x480.dtbo vc4-kms-dsi-st7701-480x480.dts
sudo cp vc4-kms-dsi-st7701-480x480.dtbo /boot/firmware/overlays/
sudo cp panel-st7701-480x480.ko /lib/modules/$(uname -r)/kernel/drivers/gpu/drm/panel/
sudo depmod -a
```

# 5. 启用

> 编辑 /boot/firmware/config.txt，添加：

```
# 关闭自动检测，避免和手动 overlay 冲突
display_auto_detect=0

dtoverlay=vc4-kms-v3d

# 如果使用 DSI0 接口，写成：dtoverlay=vc4-kms-dsi-st7701-480x480,dsi0
dtoverlay=vc4-kms-dsi-st7701-480x480

# 忽略官方 LCD
ignore_lcd=1
```

> 重启：

```
sudo reboot
```

