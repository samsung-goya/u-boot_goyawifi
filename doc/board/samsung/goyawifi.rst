.. SPDX-License-Identifier: GPL-2.0+

Samsung Galaxy Tab 3 Lite 7.0 Wi-Fi (SM-T110, goyawifi)
=======================================================

The SM-T110 is built around the Marvell PXA986 (PXA988 family, dual
Cortex-A9 r3p0, PL310 L2, GIC) with 1 GiB of LPDDR2 at ``0x00000000``,
8 GB eMMC, a microSD slot, a 1024x600 LVDS panel behind a QuickLogic VX5B3D
DSI bridge and a Marvell 88PM822 PMIC.

U-Boot does not replace the Samsung boot chain (BootROM -> OBM -> S-Boot,
all signed). Instead it is packed into an Android boot image and flashed to
the ``KERNEL`` partition. S-Boot loads it at ``0x10008000`` and jumps to it
as if it was a Linux zImage (r0 = 0, r1 = machine id, r2 = ATAGs), with
DRAM, PLLs, PMIC and the display already initialised.

What works / what to expect
---------------------------

* Debug UART (UART1, 115200 8N1) via the micro-USB port and a 619 kOhm
  JIG cable (the RT8973A MUIC switches to UART).
* The S-Boot splash framebuffer at ``0x17000000`` is reused as a
  ``simple-framebuffer`` console (1024x600, a8r8g8b8).
* eMMC (SDHCI 3) and microSD (SDHCI 1), PXAv3 SDHCI via ``mv_sdhci``.
* Volume Up / Volume Down / Home keys (GPIO 1 / 2 / 13) as a keyboard for
  ``bootmenu``; Volume Down held at boot stops autoboot.
* USB device mode (ChipIdea + PXA988 PHY): ``fastboot`` and ``ums``.
* Power off through the 88PM822 (I2C bus 2, address 0x30) and reset
  through the AP watchdog; ``reboot_recovery`` sets RTC_BR0 so S-Boot
  boots the ``RECOVERY`` partition.
* bootstd (extlinux, EFI, scripts) and ``EFI_LOADER`` (UEFI boot).

Not done: L2 cache (kept off), DVFS, the second CPU (left to Linux),
display re-initialisation (only the S-Boot splash is reused) and charging.

Building
--------

.. code-block:: bash

    export CROSS_COMPILE=arm-linux-gnueabihf-
    make goyawifi_defconfig
    make -j$(nproc)

This produces ``u-boot.bin`` (U-Boot plus its appended control DTB). The
image starts with the ARM exception vectors followed by a zImage header
(magic ``0x016f2818`` at offset ``0x24``), so zImage-checking loaders accept
it.

Packing and flashing
--------------------

Wrap ``u-boot.bin`` into an Android v0 boot image (page size 2048, base
``0x10000000``, board name ``MRVL``) and flash it to ``KERNEL``, with Odin
(Heimdall works too) from download mode (Volume Down + Home + Power):

.. code-block:: bash

    mkbootimg-goyawifi.py -p u-boot.bin -o boot.img --odin
    heimdall flash --KERNEL boot.img        # or boot.img.tar.md5 in Odin AP

``mkbootimg-goyawifi.py`` is shipped with the goyawifi bootloader tools.
Standard AOSP ``mkbootimg`` works as well::

    mkbootimg --kernel u-boot.bin --base 0x10000000 --pagesize 2048 \
              --ramdisk_offset 0x01000000 --board MRVL -o boot.img

Keep the stock or a custom recovery in ``RECOVERY``; it remains reachable
(Volume Up + Home + Power, or ``reboot_recovery`` from U-Boot).

Booting an OS
-------------

``bootcmd`` tries, in order:

1. ``bootflow scan -lb``: extlinux / boot scripts / EFI on microSD, then
   eMMC.
2. An Android boot image appended to U-Boot at 1 MiB into ``KERNEL``
   (lk2nd style, see ``mkbootimg-goyawifi.py --append``).
3. ``bootmenu``.

The bootmenu (Volume Up/Down to move, Home to select) also offers
chain-loading the UEFI firmware from ``edk2-goyawifi``: ``boot_edk2`` loads
``GOYAWIFI_EFI.fd`` from the first partition of the microSD card (then eMMC)
to ``0x10008000`` and starts it with ``bootz``; the FD carries a zImage
header for exactly this purpose.

Linux must keep ``0x00000000-0x000fffff`` and ``0x06000000-0x080fffff``
reserved (S-Boot hardware-protects the CP area). The goyawifi 4.19 kernel
with ``CONFIG_ARM_SAMSUNG_PXA_BOOTFIXUP=y`` handles this (it decompresses to
physical ``0x00108000`` wherever it was loaded) and can be booted from
``${kernel_addr_r}`` (``0x10008000``) with its own DTB.

Memory map used by U-Boot
-------------------------

=====================  ==============================================
Address                Use
=====================  ==============================================
0x00000000-0x000fffff  OBM / BootROM scratch (reserved)
0x06000000-0x080fffff  CP carveout, hardware protected (reserved)
0x08100000-0x0817ffff  ramoops / last_kmsg (reserved)
0x10008000             S-Boot load address = ``CONFIG_TEXT_BASE``,
                       later ``kernel_addr_r``
0x11f00000             ``fdt_addr_r``
0x12000000             ``ramdisk_addr_r``
0x17000000-0x17257fff  S-Boot splash / simple-framebuffer (reserved)
0x30000000             ``loadaddr`` / fastboot buffer (128 MiB)
top of DRAM            relocated U-Boot
=====================  ==============================================
