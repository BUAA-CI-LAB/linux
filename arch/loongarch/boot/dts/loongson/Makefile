# SPDX-License-Identifier: GPL-2.0
dtb-$(CONFIG_LS_SOC) += loongson32_ls.dtb
dtb-$(CONFIG_BX_SOC) += loongson32_bx.dtb
dtb-$(CONFIG_MEGA_SOC_FPGA) += la32rmega_fpga.dtb
dtb-$(CONFIG_MEGA_SOC_ASIC) += la32rmega_asic.dtb
obj-y				+= $(patsubst %.dtb, %.dtb.o, $(dtb-y))
