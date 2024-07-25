# MegaSoC 构建命令
export ARCH=loongarch; \
export CROSS_COMPILE=loongarch32r-linux-gnusf-; \
clear && make la32rmega_defconfig && make -j`nproc`
