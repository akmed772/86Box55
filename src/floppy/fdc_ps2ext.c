/*
 * 86Box     A hypervisor and IBM PC system emulator that specializes in
 *           running old operating systems and software designed for IBM
 *           PC systems and compatibles from 1981 through fairly recent
 *           system designs based on the PCI bus.
 *
 *           This file is part of the 86Box distribution.
 *
 *           Emulation of IBM 5.25" Diskette Adapter for PS/2 and PS/55 MCA machines.
 *
 * Authors:  Akamaki
 *
 *           Copyright 2025 Akamaki
 */

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <wchar.h>
#define HAVE_STDARG_H
#include <86box/86box.h>
#include <86box/device.h>
#include <86box/io.h>
#include <86box/mca.h>
#include <86box/mem.h>
#include <86box/rom.h>
#include <86box/machine.h>
#include <86box/timer.h>
#include <86box/fdd.h>
#include <86box/fdc.h>
#include <86box/fdc_ext.h>
#include <86box/plat_unused.h>

#define FDCEXT525_IOBASE 0x280
#define FDCEXT525_IRQ    0x6
#define FDCEXT525_DMA    0x3

typedef struct ps2ext525_s {
    fdc_t *fdc;
    uint8_t pos_regs[8];
} ps2ext525_t;

uint8_t
ps2ext525_mca_read(int port, void *p)
{
    ps2ext525_t *dev = (ps2ext525_t *) p;
    pclog("FDC_PS2EXT: mca_read port %x, ret %x\n", port, dev->pos_regs[port & 7]);
    return dev->pos_regs[port & 7];
}

void
ps2ext525_mca_write(int port, uint8_t val, void *p)
{
    ps2ext525_t *dev = (ps2ext525_t *) p;

    pclog("FDC_PS2EXT: mca_write port %x, val %x\n", port, val);
    if (port != 0x102)
        return;
    dev->pos_regs[port & 7] = val;
    if ((port & 7) == 2) {
        fdc_remove(dev->fdc);
        fdc_set_dma_ch(dev->fdc, (dev->pos_regs[2] >> 4) & 7);
        if (dev->pos_regs[2] & 0x01) {
            fdc_set_base(dev->fdc, FDCEXT525_IOBASE);
        }
    }
}

static uint8_t
ps2ext525_mca_feedb(void *p)
{
    const ps2ext525_t *dev = (ps2ext525_t *) p;

    return dev->pos_regs[2] & 0x01;
}

static void
ps2ext525_mca_reset(void *p)
{
    ps2ext525_t *dev = (ps2ext525_t *) p;
    fdc_reset(dev->fdc);
}

static void
ps2ext525_close(void *p)
{
    ps2ext525_t *dev = (ps2ext525_t *) p;

    free(dev);
}

static void *
ps2ext525_init(const device_t *info)
{

    ps2ext525_t *dev = calloc(1, sizeof(ps2ext525_t));

    dev->fdc = device_add(&fdc_ps2_ext_device);

    fdc_set_irq(dev->fdc, FDCEXT525_IRQ);
    fdc_set_dma_ch(dev->fdc, FDCEXT525_DMA);
    fdc_set_base(dev->fdc, FDCEXT525_IOBASE);
    fdc_remove(dev->fdc);

    /* Set the MCA ID for this controller. */
    dev->pos_regs[0] = 0xFA;
    dev->pos_regs[1] = 0xDF;
    dev->pos_regs[2] = 0x3E;
    dev->pos_regs[3] = 0x04;
    dev->pos_regs[4] = 0x04;
    /*
    I/O 283h Bit 7 1:Ext (Drive 2, 3) / 0:Int (Drive 0, 1)
    POS4 Bit 0 External 1st
         Bit 1 External 2nd
         Bit 2 Internal 1st
         Bit 3 Internal 2nd
    POS4 Bit 0 and Bit 2 = 1 then err
    POS4 Bit 0 = 1 then POS3 Bit 6 = 0 then err
         else           POS3 Bit 2 = 0 then err
    POS3 Bit 0 Bit 1 Bit 4 Bit 5 != 0 then err
    */

    mca_add(ps2ext525_mca_read, ps2ext525_mca_write, ps2ext525_mca_feedb, ps2ext525_mca_reset, dev);
    return dev;
}

const device_t fdc_ps2_ext_adapter_device = {
    .name          = "IBM 5.25-inch Diskette Adapter",
    .internal_name = "ps2ext525",
    .flags         = DEVICE_MCA,
    .local         = 0,
    .init          = ps2ext525_init,
    .close         = ps2ext525_close,
    .reset         = NULL,
    .available     = NULL,
    .speed_changed = NULL,
    .force_redraw  = NULL,
    .config        = NULL
};
