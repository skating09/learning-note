# imx6u_uboot源码理解

## 1、对比nxp_evk和正点原子6ull(emmc), config文件

![image-20240218095804549](D:\learning_notes\linux\Image_collection\image-20240218095804549.png)

![image-20240218095824595](D:\learning_notes\linux\Image_collection\image-20240218095824595.png)

deconfig文件在 configs\mx6ull_14x14_evk_emmc_defconfig 中。

主要区别在于（正点原子在原来的基础上）添加板，如：

```shell
#configs\mx6ull_14x14_ddr512_emmc_defconfig：
CONFIG_SYS_EXTRA_OPTIONS="IMX_CONFIG=board/freescale/mx6ullevk/imximage-ddr512.cfg,MX6ULL_EVK_EMMC_REWORK"
CONFIG_ARM=y
CONFIG_ARCH_MX6=y
CONFIG_TARGET_MX6ULL_14X14_EVK=y
CONFIG_CMD_GPIO=y

```

```shell
#configs\mx6ull_14x14_evk_emmc_defconfig：
CONFIG_SYS_EXTRA_OPTIONS="IMX_CONFIG=board/freescale/mx6ullevk/imximage.cfg,MX6ULL_EVK_EMMC_REWORK"
CONFIG_ARM=y
CONFIG_ARCH_MX6=y
CONFIG_TARGET_MX6ULL_14X14_EVK=y
CONFIG_CMD_GPIO=y

```

重点在 board/freescale/mx6ullevk/ 下的文件。当然我也对比了。
正点的 board\freescale\mx6ullevk\imximage-ddr512.cfg 
nxp的 board\freescale\mx6ullevk\imximage.cfg
发现就只有ddr的寄存器设计值不一样，其他都相同，而这个ddr的值是根据不同厂商的ddr给的资料修改而成。其他基本是一样的。

![image-20240218100435363](D:\learning_notes\linux\Image_collection\image-20240218100435363.png)

想了解这里的值的意义，请自行参考：imx6u参考手册，然后找寄存器就行了 ，比如搜索 “21B_080C”

我猜想既然uboot是用来引导启动Linux的，那么它应该是会提前把所有用到的设备都初始化好，以便开发时直接调用（可能不正确，因为uboot它可以做升级也会当启动完成后就会寿终正寝！那初始化好的外设还能在Linux系统中使用么？）

您的理解有一些偏差。uboot是一个通用的启动程序，它的主要作用是启动操作系统内核，但它并不会初始化所有的设备，只会初始化一些必要的设备，比如内存、串口、Flash等。uboot还可以提供一些系统部署和硬件管理的功能，比如烧写镜像、设置环境变量、测试网络等。

**uboot启动内核后，就会结束自己的生命周期，不会再运行。uboot初始化好的外设，如果在Linux系统中也需要使用，就需要在Linux内核中再次初始化，或者使用uboot传递给内核的参数来配置。**

```shell
#配置板 配置文件
make mx6ull_14x14_ddr512_emmc_defconfig  

```

还没完！

```
board\freescale\mx6ullevk\mx6ullevk.c
这个文件包含需要添加网口（初始化需要重启操作，类似初始化过程的操作，别忘了，uboot必须保证能联网，调试过程中，设备树，Linux镜像都需要tftp下载，根文件系统需要nfs挂载呢！！！）。还有是lcd rgb 屏幕参数这些，主要用于显示。
```

