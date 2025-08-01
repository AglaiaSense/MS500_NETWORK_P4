
#ifndef __REG_TABLE_H__
#define __REG_TABLE_H__

#include <stdint.h>
#include "esp_err.h"
#include "as_tools.h"

static const imx501_reg_t imx501_reg_table[] = {
    // Initial settings
    {0x0136, 0x1b00, 2},
    {0xd005, 0x2, 1},
    {0x0202, 0x1194, 2},
    {0x0204, 0x356, 2},
    {0xd815, 0x1, 1},
    {0xd850, 0x1, 2},
    {0xd852, 0x0, 2},
    {0xd854, 0x3, 2},
    {0xd856, 0x2, 2},
    {0xd858, 0x5, 2},
    {0xd85a, 0x4, 2},
    {0xd85c, 0x7, 2},
    {0xd85e, 0x6, 2},
    {0xd860, 0xa, 2},
    {0xd862, 0x8, 2},
    {0xd864, 0xe, 2},
    {0xd866, 0xc, 2},
    {0xd868, 0x18, 2},
    {0xd86a, 0x10, 2},
    {0xd86c, 0x26, 2},
    {0xd86e, 0x1f, 2},
    {0xd870, 0x34, 2},
    {0xd872, 0x2d, 2},
    {0xd874, 0x47, 2},
    {0xd876, 0x3a, 2},
    {0xd878, 0x5d, 2},
    {0xd87a, 0x52, 2},
    {0xd87c, 0x7a, 2},
    {0xd87e, 0x67, 2},
    {0xd880, 0x98, 2},
    {0xd882, 0x8a, 2},
    {0xd884, 0xaf, 2},
    {0xd886, 0xa4, 2},
    {0xd888, 0xc0, 2},
    {0xd88a, 0xb8, 2},
    {0xd88c, 0xcd, 2},
    {0xd88e, 0xc7, 2},
    {0xd890, 0xd7, 2},
    {0xd892, 0xd3, 2},
    {0xd894, 0xe2, 2},
    {0xd896, 0xdc, 2},
    {0xd898, 0xec, 2},
    {0xd89a, 0xe8, 2},
    {0xd89c, 0xf4, 2},
    {0xd89e, 0xf0, 2},
    {0xd8a0, 0xfb, 2},
    {0xd8a2, 0xf8, 2},
    {0xd8a4, 0xfe, 2},
    {0xd8a8, 0x1, 2},
    {0xd8aa, 0x0, 2},
    {0xd8ac, 0x3, 2},
    {0xd8ae, 0x2, 2},
    {0xd8b0, 0x5, 2},
    {0xd8b2, 0x4, 2},
    {0xd8b4, 0x7, 2},
    {0xd8b6, 0x6, 2},
    {0xd8b8, 0xa, 2},
    {0xd8ba, 0x8, 2},
    {0xd8bc, 0xe, 2},
    {0xd8be, 0xc, 2},
    {0xd8c0, 0x18, 2},
    {0xd8c2, 0x10, 2},
    {0xd8c4, 0x26, 2},
    {0xd8c6, 0x1f, 2},
    {0xd8c8, 0x34, 2},
    {0xd8ca, 0x2d, 2},
    {0xd8cc, 0x47, 2},
    {0xd8ce, 0x3a, 2},
    {0xd8d0, 0x5d, 2},
    {0xd8d2, 0x52, 2},
    {0xd8d4, 0x7a, 2},
    {0xd8d6, 0x67, 2},
    {0xd8d8, 0x98, 2},
    {0xd8da, 0x8a, 2},
    {0xd8dc, 0xaf, 2},
    {0xd8de, 0xa4, 2},
    {0xd8e0, 0xc0, 2},
    {0xd8e2, 0xb8, 2},
    {0xd8e4, 0xcd, 2},
    {0xd8e6, 0xc7, 2},
    {0xd8e8, 0xd7, 2},
    {0xd8ea, 0xd3, 2},
    {0xd8ec, 0xe2, 2},
    {0xd8ee, 0xdc, 2},
    {0xd8f0, 0xec, 2},
    {0xd8f2, 0xe8, 2},
    {0xd8f4, 0xf4, 2},
    {0xd8f6, 0xf0, 2},
    {0xd8f8, 0xfb, 2},
    {0xd8fa, 0xf8, 2},
    {0xd8fc, 0xfe, 2},
    {0xd900, 0x1, 2},
    {0xd902, 0x0, 2},
    {0xd904, 0x3, 2},
    {0xd906, 0x2, 2},
    {0xd908, 0x5, 2},
    {0xd90a, 0x4, 2},
    {0xd90c, 0x7, 2},
    {0xd90e, 0x6, 2},
    {0xd910, 0xa, 2},
    {0xd912, 0x8, 2},
    {0xd914, 0xe, 2},
    {0xd916, 0xc, 2},
    {0xd918, 0x18, 2},
    {0xd91a, 0x10, 2},
    {0xd91c, 0x26, 2},
    {0xd91e, 0x1f, 2},
    {0xd920, 0x34, 2},
    {0xd922, 0x2d, 2},
    {0xd924, 0x47, 2},
    {0xd926, 0x3a, 2},
    {0xd928, 0x5d, 2},
    {0xd92a, 0x52, 2},
    {0xd92c, 0x7a, 2},
    {0xd92e, 0x67, 2},
    {0xd930, 0x98, 2},
    {0xd932, 0x8a, 2},
    {0xd934, 0xaf, 2},
    {0xd936, 0xa4, 2},
    {0xd938, 0xc0, 2},
    {0xd93a, 0xb8, 2},
    {0xd93c, 0xcd, 2},
    {0xd93e, 0xc7, 2},
    {0xd940, 0xd7, 2},
    {0xd942, 0xd3, 2},
    {0xd944, 0xe2, 2},
    {0xd946, 0xdc, 2},
    {0xd948, 0xec, 2},
    {0xd94a, 0xe8, 2},
    {0xd94c, 0xf4, 2},
    {0xd94e, 0xf0, 2},
    {0xd950, 0xfb, 2},
    {0xd952, 0xf8, 2},
    {0xd954, 0xfe, 2},
    {0xd814, 0x1, 1},
    {0xd828, 0x0, 4},
    {0xd82c, 0x0, 4},
    {0xd830, 0x0, 4},
    {0xd834, 0x0, 4},
    {0xd838, 0x0, 4},
    {0xd83c, 0x0, 4},
    {0xd840, 0x0, 4},
    {0xd844, 0x0, 4},
    {0xd848, 0x0, 4},
    {0xd826, 0x1, 1},
    {0xd964, 0x0, 2},
    {0xd966, 0x0, 2},
    {0xd968, 0x0, 2},
    {0xd96a, 0x0, 2},
    {0xd970, 0x0, 2},
    {0xd972, 0x0, 2},
    {0xd974, 0x0, 2},
    {0xd976, 0x0, 2},
    {0xd80c, 0x100, 2},
    {0xd80e, 0x100, 2},
    {0xd810, 0x100, 2},
    {0xd812, 0x100, 2},
    {0xd822, 0x1, 1},
    {0xd823, 0xf, 1},
    {0xd502, 0x0, 2},
    {0xd506, 0xfd8, 2},
    {0xd500, 0x0, 2},
    {0xd504, 0xbe0, 2},
    {0xd680, 0x0, 1},
    {0xd800, 0x0, 1},
    {0xd23d, 0x0, 1},
    {0xd260, 0x1888, 2},
    {0xd100, 0x4, 1},
    {0x3607, 0x1, 1},
    {0x3e34, 0x10100, 4},
    {0x3e38, 0x1010100, 4},
    {0x4bc0, 0x16, 1},
    {0x7ba8, 0x0, 2},
    {0x886b, 0x0, 1},
    {0xd804, 0x32a, 2},
    {0xd80a, 0x1cc, 2},
    {0xbcff, 0x1, 1},
    {0xbcf1, 0x2, 1},
    {0x0114, 0x1, 1},
    {0xb34e, 0x0, 1},
    {0xb351, 0x20, 1},
    {0xb35c, 0x0, 1},
    {0xb35e, 0x8, 1},
    {0x0408, 0x0, 2},
    {0x040a, 0x0, 2},

    // E000-E600 series registers (gamma/color correction tables)
    {0xe000, 0x2fa, 2},
    {0xe002, 0x2a5, 2},
    {0xe004, 0x21f, 2},
    {0xe006, 0x1ec, 2},
    {0xe008, 0x1b4, 2},
    {0xe00a, 0x192, 2},
    {0xe00c, 0x17c, 2},
    {0xe00e, 0x164, 2},
    {0xe010, 0x15c, 2},
    {0xe012, 0x14b, 2},
    {0xe014, 0x14d, 2},
    {0xe016, 0x140, 2},
    {0xe018, 0x147, 2},
    {0xe01a, 0x13b, 2},
    {0xe01c, 0x144, 2},
    {0xe01e, 0x139, 2},
    {0xe020, 0x145, 2},
    {0xe022, 0x13a, 2},
    {0xe024, 0x148, 2},
    {0xe026, 0x13c, 2},
    {0xe028, 0x150, 2},
    {0xe02a, 0x142, 2},
    {0xe02c, 0x162, 2},
    {0xe02e, 0x14f, 2},
    {0xe030, 0x187, 2},
    {0xe032, 0x16e, 2},
    {0xe034, 0x1c7, 2},
    {0xe036, 0x1a3, 2},
    {0xe038, 0x241, 2},
    {0xe03a, 0x20c, 2},
    {0xe03c, 0x340, 2},
    {0xe03e, 0x2eb, 2},
    {0xe040, 0x246, 2},
    {0xe042, 0x20d, 2},
    {0xe044, 0x1bd, 2},
    {0xe046, 0x19a, 2},
    {0xe048, 0x178, 2},
    {0xe04a, 0x161, 2},
    {0xe04c, 0x154, 2},
    {0xe04e, 0x144, 2},
    {0xe050, 0x144, 2},
    {0xe052, 0x139, 2},
    {0xe054, 0x13a, 2},
    {0xe056, 0x131, 2},
    {0xe058, 0x134, 2},
    {0xe05a, 0x12c, 2},
    {0xe05c, 0x131, 2},
    {0xe05e, 0x12a, 2},
    {0xe060, 0x131, 2},
    {0xe062, 0x12a, 2},
    {0xe064, 0x135, 2},
    {0xe066, 0x12d, 2},
    {0xe068, 0x13d, 2},
    {0xe06a, 0x133, 2},
    {0xe06c, 0x148, 2},
    {0xe06e, 0x13b, 2},
    {0xe070, 0x15c, 2},
    {0xe072, 0x14a, 2},
    {0xe074, 0x185, 2},
    {0xe076, 0x16c, 2},
    {0xe078, 0x1d4, 2},
    {0xe07a, 0x1af, 2},
    {0xe07c, 0x273, 2},
    {0xe07e, 0x238, 2},
    {0xe080, 0x1e7, 2},
    {0xe082, 0x1bc, 2},
    {0xe084, 0x188, 2},
    {0xe086, 0x16e, 2},
    {0xe088, 0x157, 2},
    {0xe08a, 0x147, 2},
    {0xe08c, 0x143, 2},
    {0xe08e, 0x137, 2},
    {0xe090, 0x135, 2},
    {0xe092, 0x12d, 2},
    {0xe094, 0x12a, 2},
    {0xe096, 0x124, 2},
    {0xe098, 0x123, 2},
    {0xe09a, 0x11e, 2},
    {0xe09c, 0x11f, 2},
    {0xe09e, 0x11b, 2},
    {0xe0a0, 0x120, 2},
    {0xe0a2, 0x11b, 2},
    {0xe0a4, 0x124, 2},
    {0xe0a6, 0x11f, 2},
    {0xe0a8, 0x12c, 2},
    {0xe0aa, 0x126, 2},
    {0xe0ac, 0x138, 2},
    {0xe0ae, 0x12f, 2},
    {0xe0b0, 0x147, 2},
    {0xe0b2, 0x13b, 2},
    {0xe0b4, 0x160, 2},
    {0xe0b6, 0x14e, 2},
    {0xe0b8, 0x199, 2},
    {0xe0ba, 0x17c, 2},
    {0xe0bc, 0x208, 2},
    {0xe0be, 0x1db, 2},
    {0xe0c0, 0x1b3, 2},
    {0xe0c2, 0x190, 2},
    {0xe0c4, 0x169, 2},
    {0xe0c6, 0x155, 2},
    {0xe0c8, 0x148, 2},
    {0xe0ca, 0x13b, 2},
    {0xe0cc, 0x137, 2},
    {0xe0ce, 0x12e, 2},
    {0xe0d0, 0x128, 2},
    {0xe0d2, 0x122, 2},
    {0xe0d4, 0x11d, 2},
    {0xe0d6, 0x119, 2},
    {0xe0d8, 0x115, 2},
    {0xe0da, 0x112, 2},
    {0xe0dc, 0x111, 2},
    {0xe0de, 0x10f, 2},
    {0xe0e0, 0x112, 2},
    {0xe0e2, 0x110, 2},
    {0xe0e4, 0x116, 2},
    {0xe0e6, 0x114, 2},
    {0xe0e8, 0x11f, 2},
    {0xe0ea, 0x11b, 2},
    {0xe0ec, 0x12c, 2},
    {0xe0ee, 0x125, 2},
    {0xe0f0, 0x13c, 2},
    {0xe0f2, 0x132, 2},
    {0xe0f4, 0x14e, 2},
    {0xe0f6, 0x140, 2},
    {0xe0f8, 0x176, 2},
    {0xe0fa, 0x160, 2},
    {0xe0fc, 0x1cc, 2},
    {0xe0fe, 0x1a8, 2},
    {0xe100, 0x196, 2},
    {0xe102, 0x179, 2},
    {0xe104, 0x159, 2},
    {0xe106, 0x148, 2},
    {0xe108, 0x140, 2},
    {0xe10a, 0x136, 2},
    {0xe10c, 0x12f, 2},
    {0xe10e, 0x128, 2},
    {0xe110, 0x120, 2},
    {0xe112, 0x11b, 2},
    {0xe114, 0x113, 2},
    {0xe116, 0x111, 2},
    {0xe118, 0x10b, 2},
    {0xe11a, 0x10a, 2},
    {0xe11c, 0x107, 2},
    {0xe11e, 0x106, 2},
    {0xe120, 0x108, 2},
    {0xe122, 0x107, 2},
    {0xe124, 0x10c, 2},
    {0xe126, 0x10b, 2},
    {0xe128, 0x115, 2},
    {0xe12a, 0x113, 2},
    {0xe12c, 0x123, 2},
    {0xe12e, 0x11e, 2},
    {0xe130, 0x133, 2},
    {0xe132, 0x12b, 2},
    {0xe134, 0x146, 2},
    {0xe136, 0x13a, 2},
    {0xe138, 0x164, 2},
    {0xe13a, 0x151, 2},
    {0xe13c, 0x1ab, 2},
    {0xe13e, 0x18c, 2},
    {0xe140, 0x188, 2},
    {0xe142, 0x16e, 2},
    {0xe144, 0x152, 2},
    {0xe146, 0x143, 2},
    {0xe148, 0x13c, 2},
    {0xe14a, 0x132, 2},
    {0xe14c, 0x12a, 2},
    {0xe14e, 0x124, 2},
    {0xe150, 0x11a, 2},
    {0xe152, 0x117, 2},
    {0xe154, 0x10e, 2},
    {0xe156, 0x10c, 2},
    {0xe158, 0x105, 2},
    {0xe15a, 0x105, 2},
    {0xe15c, 0x101, 2},
    {0xe15e, 0x101, 2},
    {0xe160, 0x101, 2},
    {0xe162, 0x102, 2},
    {0xe164, 0x107, 2},
    {0xe166, 0x106, 2},
    {0xe168, 0x110, 2},
    {0xe16a, 0x10e, 2},
    {0xe16c, 0x11e, 2},
    {0xe16e, 0x11a, 2},
    {0xe170, 0x12e, 2},
    {0xe172, 0x127, 2},
    {0xe174, 0x140, 2},
    {0xe176, 0x136, 2},
    {0xe178, 0x15a, 2},
    {0xe17a, 0x14a, 2},
    {0xe17c, 0x19a, 2},
    {0xe17e, 0x17f, 2},
    {0xe180, 0x185, 2},
    {0xe182, 0x16b, 2},
    {0xe184, 0x150, 2},
    {0xe186, 0x141, 2},
    {0xe188, 0x13b, 2},
    {0xe18a, 0x131, 2},
    {0xe18c, 0x129, 2},
    {0xe18e, 0x123, 2},
    {0xe190, 0x119, 2},
    {0xe192, 0x116, 2},
    {0xe194, 0x10d, 2},
    {0xe196, 0x10b, 2},
    {0xe198, 0x104, 2},
    {0xe19a, 0x103, 2},
    {0xe19c, 0x100, 2},
    {0xe19e, 0x100, 2},
    {0xe1a0, 0x101, 2},
    {0xe1a2, 0x101, 2},
    {0xe1a4, 0x107, 2},
    {0xe1a6, 0x106, 2},
    {0xe1a8, 0x10f, 2},
    {0xe1aa, 0x10e, 2},
    {0xe1ac, 0x11c, 2},
    {0xe1ae, 0x119, 2},
    {0xe1b0, 0x12d, 2},
    {0xe1b2, 0x126, 2},
    {0xe1b4, 0x13f, 2},
    {0xe1b6, 0x135, 2},
    {0xe1b8, 0x159, 2},
    {0xe1ba, 0x148, 2},
    {0xe1bc, 0x198, 2},
    {0xe1be, 0x17d, 2},
    {0xe1c0, 0x18d, 2},
    {0xe1c2, 0x172, 2},
    {0xe1c4, 0x153, 2},
    {0xe1c6, 0x144, 2},
    {0xe1c8, 0x13c, 2},
    {0xe1ca, 0x132, 2},
    {0xe1cc, 0x12b, 2},
    {0xe1ce, 0x125, 2},
    {0xe1d0, 0x11c, 2},
    {0xe1d2, 0x118, 2},
    {0xe1d4, 0x110, 2},
    {0xe1d6, 0x10e, 2},
    {0xe1d8, 0x107, 2},
    {0xe1da, 0x106, 2},
    {0xe1dc, 0x104, 2},
    {0xe1de, 0x103, 2},
    {0xe1e0, 0x104, 2},
    {0xe1e2, 0x104, 2},
    {0xe1e4, 0x109, 2},
    {0xe1e6, 0x108, 2},
    {0xe1e8, 0x112, 2},
    {0xe1ea, 0x110, 2},
    {0xe1ec, 0x11f, 2},
    {0xe1ee, 0x11b, 2},
    {0xe1f0, 0x12f, 2},
    {0xe1f2, 0x128, 2},
    {0xe1f4, 0x141, 2},
    {0xe1f6, 0x136, 2},
    {0xe1f8, 0x15c, 2},
    {0xe1fa, 0x14b, 2},
    {0xe1fc, 0x1a0, 2},
    {0xe1fe, 0x184, 2},
    {0xe200, 0x1a4, 2},
    {0xe202, 0x184, 2},
    {0xe204, 0x160, 2},
    {0xe206, 0x14d, 2},
    {0xe208, 0x143, 2},
    {0xe20a, 0x137, 2},
    {0xe20c, 0x132, 2},
    {0xe20e, 0x12a, 2},
    {0xe210, 0x123, 2},
    {0xe212, 0x11e, 2},
    {0xe214, 0x118, 2},
    {0xe216, 0x114, 2},
    {0xe218, 0x10f, 2},
    {0xe21a, 0x10d, 2},
    {0xe21c, 0x10c, 2},
    {0xe21e, 0x10a, 2},
    {0xe220, 0x10c, 2},
    {0xe222, 0x10b, 2},
    {0xe224, 0x111, 2},
    {0xe226, 0x10f, 2},
    {0xe228, 0x119, 2},
    {0xe22a, 0x117, 2},
    {0xe22c, 0x126, 2},
    {0xe22e, 0x121, 2},
    {0xe230, 0x135, 2},
    {0xe232, 0x12d, 2},
    {0xe234, 0x147, 2},
    {0xe236, 0x13a, 2},
    {0xe238, 0x16a, 2},
    {0xe23a, 0x156, 2},
    {0xe23c, 0x1b9, 2},
    {0xe23e, 0x198, 2},
    {0xe240, 0x1cd, 2},
    {0xe242, 0x1a5, 2},
    {0xe244, 0x177, 2},
    {0xe246, 0x15f, 2},
    {0xe248, 0x14e, 2},
    {0xe24a, 0x13f, 2},
    {0xe24c, 0x13c, 2},
    {0xe24e, 0x132, 2},
    {0xe250, 0x12e, 2},
    {0xe252, 0x127, 2},
    {0xe254, 0x123, 2},
    {0xe256, 0x11e, 2},
    {0xe258, 0x11c, 2},
    {0xe25a, 0x117, 2},
    {0xe25c, 0x118, 2},
    {0xe25e, 0x114, 2},
    {0xe260, 0x118, 2},
    {0xe262, 0x115, 2},
    {0xe264, 0x11d, 2},
    {0xe266, 0x119, 2},
    {0xe268, 0x125, 2},
    {0xe26a, 0x120, 2},
    {0xe26c, 0x131, 2},
    {0xe26e, 0x129, 2},
    {0xe270, 0x13f, 2},
    {0xe272, 0x135, 2},
    {0xe274, 0x154, 2},
    {0xe276, 0x144, 2},
    {0xe278, 0x185, 2},
    {0xe27a, 0x16c, 2},
    {0xe27c, 0x1e7, 2},
    {0xe27e, 0x1bf, 2},
    {0xe280, 0x217, 2},
    {0xe282, 0x1e2, 2},
    {0xe284, 0x1a1, 2},
    {0xe286, 0x181, 2},
    {0xe288, 0x165, 2},
    {0xe28a, 0x151, 2},
    {0xe28c, 0x149, 2},
    {0xe28e, 0x13b, 2},
    {0xe290, 0x13c, 2},
    {0xe292, 0x132, 2},
    {0xe294, 0x132, 2},
    {0xe296, 0x12a, 2},
    {0xe298, 0x12b, 2},
    {0xe29a, 0x124, 2},
    {0xe29c, 0x127, 2},
    {0xe29e, 0x122, 2},
    {0xe2a0, 0x128, 2},
    {0xe2a2, 0x122, 2},
    {0xe2a4, 0x12b, 2},
    {0xe2a6, 0x125, 2},
    {0xe2a8, 0x133, 2},
    {0xe2aa, 0x12b, 2},
    {0xe2ac, 0x13e, 2},
    {0xe2ae, 0x134, 2},
    {0xe2b0, 0x14d, 2},
    {0xe2b2, 0x13f, 2},
    {0xe2b4, 0x16f, 2},
    {0xe2b6, 0x15a, 2},
    {0xe2b8, 0x1b2, 2},
    {0xe2ba, 0x192, 2},
    {0xe2bc, 0x23a, 2},
    {0xe2be, 0x206, 2},
    {0xe2c0, 0x29d, 2},
    {0xe2c2, 0x253, 2},
    {0xe2c4, 0x1eb, 2},
    {0xe2c6, 0x1bf, 2},
    {0xe2c8, 0x193, 2},
    {0xe2ca, 0x176, 2},
    {0xe2cc, 0x164, 2},
    {0xe2ce, 0x151, 2},
    {0xe2d0, 0x14d, 2},
    {0xe2d2, 0x13f, 2},
    {0xe2d4, 0x143, 2},
    {0xe2d6, 0x138, 2},
    {0xe2d8, 0x13c, 2},
    {0xe2da, 0x133, 2},
    {0xe2dc, 0x13a, 2},
    {0xe2de, 0x131, 2},
    {0xe2e0, 0x13a, 2},
    {0xe2e2, 0x131, 2},
    {0xe2e4, 0x13d, 2},
    {0xe2e6, 0x134, 2},
    {0xe2e8, 0x144, 2},
    {0xe2ea, 0x139, 2},
    {0xe2ec, 0x151, 2},
    {0xe2ee, 0x143, 2},
    {0xe2f0, 0x16c, 2},
    {0xe2f2, 0x158, 2},
    {0xe2f4, 0x1a1, 2},
    {0xe2f6, 0x184, 2},
    {0xe2f8, 0x204, 2},
    {0xe2fa, 0x1d8, 2},
    {0xe2fc, 0x2d2, 2},
    {0xe2fe, 0x289, 2},
    {0xe300, 0x2a4, 2},
    {0xe302, 0x27f, 2},
    {0xe304, 0x1ea, 2},
    {0xe306, 0x1d3, 2},
    {0xe308, 0x190, 2},
    {0xe30a, 0x181, 2},
    {0xe30c, 0x161, 2},
    {0xe30e, 0x157, 2},
    {0xe310, 0x148, 2},
    {0xe312, 0x140, 2},
    {0xe314, 0x13d, 2},
    {0xe316, 0x137, 2},
    {0xe318, 0x138, 2},
    {0xe31a, 0x133, 2},
    {0xe31c, 0x136, 2},
    {0xe31e, 0x131, 2},
    {0xe320, 0x136, 2},
    {0xe322, 0x132, 2},
    {0xe324, 0x139, 2},
    {0xe326, 0x134, 2},
    {0xe328, 0x13e, 2},
    {0xe32a, 0x139, 2},
    {0xe32c, 0x14b, 2},
    {0xe32e, 0x144, 2},
    {0xe330, 0x16a, 2},
    {0xe332, 0x15f, 2},
    {0xe334, 0x19e, 2},
    {0xe336, 0x190, 2},
    {0xe338, 0x206, 2},
    {0xe33a, 0x1f2, 2},
    {0xe33c, 0x2e3, 2},
    {0xe33e, 0x2c2, 2},
    {0xe340, 0x20c, 2},
    {0xe342, 0x1f6, 2},
    {0xe344, 0x199, 2},
    {0xe346, 0x18a, 2},
    {0xe348, 0x160, 2},
    {0xe34a, 0x157, 2},
    {0xe34c, 0x143, 2},
    {0xe34e, 0x13d, 2},
    {0xe350, 0x137, 2},
    {0xe352, 0x133, 2},
    {0xe354, 0x12f, 2},
    {0xe356, 0x12c, 2},
    {0xe358, 0x12a, 2},
    {0xe35a, 0x127, 2},
    {0xe35c, 0x127, 2},
    {0xe35e, 0x125, 2},
    {0xe360, 0x127, 2},
    {0xe362, 0x126, 2},
    {0xe364, 0x12b, 2},
    {0xe366, 0x128, 2},
    {0xe368, 0x131, 2},
    {0xe36a, 0x12e, 2},
    {0xe36c, 0x139, 2},
    {0xe36e, 0x135, 2},
    {0xe370, 0x147, 2},
    {0xe372, 0x143, 2},
    {0xe374, 0x169, 2},
    {0xe376, 0x161, 2},
    {0xe378, 0x1ab, 2},
    {0xe37a, 0x19e, 2},
    {0xe37c, 0x233, 2},
    {0xe37e, 0x21e, 2},
    {0xe380, 0x1bd, 2},
    {0xe382, 0x1ad, 2},
    {0xe384, 0x16d, 2},
    {0xe386, 0x163, 2},
    {0xe388, 0x146, 2},
    {0xe38a, 0x140, 2},
    {0xe38c, 0x136, 2},
    {0xe38e, 0x132, 2},
    {0xe390, 0x12b, 2},
    {0xe392, 0x129, 2},
    {0xe394, 0x123, 2},
    {0xe396, 0x120, 2},
    {0xe398, 0x11c, 2},
    {0xe39a, 0x11b, 2},
    {0xe39c, 0x119, 2},
    {0xe39e, 0x118, 2},
    {0xe3a0, 0x119, 2},
    {0xe3a2, 0x118, 2},
    {0xe3a4, 0x11d, 2},
    {0xe3a6, 0x11c, 2},
    {0xe3a8, 0x124, 2},
    {0xe3aa, 0x122, 2},
    {0xe3ac, 0x12d, 2},
    {0xe3ae, 0x12b, 2},
    {0xe3b0, 0x139, 2},
    {0xe3b2, 0x135, 2},
    {0xe3b4, 0x14c, 2},
    {0xe3b6, 0x146, 2},
    {0xe3b8, 0x17a, 2},
    {0xe3ba, 0x170, 2},
    {0xe3bc, 0x1d8, 2},
    {0xe3be, 0x1c8, 2},
    {0xe3c0, 0x192, 2},
    {0xe3c2, 0x185, 2},
    {0xe3c4, 0x155, 2},
    {0xe3c6, 0x14d, 2},
    {0xe3c8, 0x13b, 2},
    {0xe3ca, 0x137, 2},
    {0xe3cc, 0x12e, 2},
    {0xe3ce, 0x12b, 2},
    {0xe3d0, 0x122, 2},
    {0xe3d2, 0x120, 2},
    {0xe3d4, 0x118, 2},
    {0xe3d6, 0x116, 2},
    {0xe3d8, 0x111, 2},
    {0xe3da, 0x110, 2},
    {0xe3dc, 0x10e, 2},
    {0xe3de, 0x10d, 2},
    {0xe3e0, 0x10e, 2},
    {0xe3e2, 0x10e, 2},
    {0xe3e4, 0x112, 2},
    {0xe3e6, 0x112, 2},
    {0xe3e8, 0x11a, 2},
    {0xe3ea, 0x119, 2},
    {0xe3ec, 0x124, 2},
    {0xe3ee, 0x122, 2},
    {0xe3f0, 0x130, 2},
    {0xe3f2, 0x12e, 2},
    {0xe3f4, 0x13e, 2},
    {0xe3f6, 0x13b, 2},
    {0xe3f8, 0x15e, 2},
    {0xe3fa, 0x157, 2},
    {0xe3fc, 0x1a6, 2},
    {0xe3fe, 0x19a, 2},
    {0xe400, 0x17b, 2},
    {0xe402, 0x170, 2},
    {0xe404, 0x149, 2},
    {0xe406, 0x143, 2},
    {0xe408, 0x136, 2},
    {0xe40a, 0x132, 2},
    {0xe40c, 0x128, 2},
    {0xe40e, 0x125, 2},
    {0xe410, 0x11b, 2},
    {0xe412, 0x119, 2},
    {0xe414, 0x110, 2},
    {0xe416, 0x10f, 2},
    {0xe418, 0x109, 2},
    {0xe41a, 0x108, 2},
    {0xe41c, 0x106, 2},
    {0xe41e, 0x105, 2},
    {0xe420, 0x106, 2},
    {0xe422, 0x106, 2},
    {0xe424, 0x10a, 2},
    {0xe426, 0x10a, 2},
    {0xe428, 0x112, 2},
    {0xe42a, 0x111, 2},
    {0xe42c, 0x11d, 2},
    {0xe42e, 0x11c, 2},
    {0xe430, 0x12a, 2},
    {0xe432, 0x129, 2},
    {0xe434, 0x139, 2},
    {0xe436, 0x136, 2},
    {0xe438, 0x151, 2},
    {0xe43a, 0x14b, 2},
    {0xe43c, 0x18c, 2},
    {0xe43e, 0x181, 2},
    {0xe440, 0x170, 2},
    {0xe442, 0x166, 2},
    {0xe444, 0x144, 2},
    {0xe446, 0x13e, 2},
    {0xe448, 0x133, 2},
    {0xe44a, 0x12f, 2},
    {0xe44c, 0x124, 2},
    {0xe44e, 0x122, 2},
    {0xe450, 0x117, 2},
    {0xe452, 0x115, 2},
    {0xe454, 0x10c, 2},
    {0xe456, 0x10b, 2},
    {0xe458, 0x104, 2},
    {0xe45a, 0x104, 2},
    {0xe45c, 0x101, 2},
    {0xe45e, 0x101, 2},
    {0xe460, 0x101, 2},
    {0xe462, 0x101, 2},
    {0xe464, 0x106, 2},
    {0xe466, 0x106, 2},
    {0xe468, 0x10e, 2},
    {0xe46a, 0x10d, 2},
    {0xe46c, 0x11a, 2},
    {0xe46e, 0x119, 2},
    {0xe470, 0x127, 2},
    {0xe472, 0x125, 2},
    {0xe474, 0x136, 2},
    {0xe476, 0x133, 2},
    {0xe478, 0x14a, 2},
    {0xe47a, 0x144, 2},
    {0xe47c, 0x17f, 2},
    {0xe47e, 0x175, 2},
    {0xe480, 0x16e, 2},
    {0xe482, 0x164, 2},
    {0xe484, 0x143, 2},
    {0xe486, 0x13d, 2},
    {0xe488, 0x132, 2},
    {0xe48a, 0x12f, 2},
    {0xe48c, 0x124, 2},
    {0xe48e, 0x121, 2},
    {0xe490, 0x116, 2},
    {0xe492, 0x115, 2},
    {0xe494, 0x10b, 2},
    {0xe496, 0x10a, 2},
    {0xe498, 0x104, 2},
    {0xe49a, 0x103, 2},
    {0xe49c, 0x100, 2},
    {0xe49e, 0x100, 2},
    {0xe4a0, 0x101, 2},
    {0xe4a2, 0x101, 2},
    {0xe4a4, 0x106, 2},
    {0xe4a6, 0x106, 2},
    {0xe4a8, 0x10d, 2},
    {0xe4aa, 0x10d, 2},
    {0xe4ac, 0x119, 2},
    {0xe4ae, 0x118, 2},
    {0xe4b0, 0x126, 2},
    {0xe4b2, 0x125, 2},
    {0xe4b4, 0x135, 2},
    {0xe4b6, 0x132, 2},
    {0xe4b8, 0x149, 2},
    {0xe4ba, 0x144, 2},
    {0xe4bc, 0x17d, 2},
    {0xe4be, 0x174, 2},
    {0xe4c0, 0x175, 2},
    {0xe4c2, 0x16b, 2},
    {0xe4c4, 0x146, 2},
    {0xe4c6, 0x140, 2},
    {0xe4c8, 0x134, 2},
    {0xe4ca, 0x130, 2},
    {0xe4cc, 0x126, 2},
    {0xe4ce, 0x123, 2},
    {0xe4d0, 0x119, 2},
    {0xe4d2, 0x117, 2},
    {0xe4d4, 0x10e, 2},
    {0xe4d6, 0x10d, 2},
    {0xe4d8, 0x107, 2},
    {0xe4da, 0x106, 2},
    {0xe4dc, 0x104, 2},
    {0xe4de, 0x103, 2},
    {0xe4e0, 0x104, 2},
    {0xe4e2, 0x104, 2},
    {0xe4e4, 0x108, 2},
    {0xe4e6, 0x108, 2},
    {0xe4e8, 0x110, 2},
    {0xe4ea, 0x10f, 2},
    {0xe4ec, 0x11b, 2},
    {0xe4ee, 0x11a, 2},
    {0xe4f0, 0x128, 2},
    {0xe4f2, 0x127, 2},
    {0xe4f4, 0x136, 2},
    {0xe4f6, 0x134, 2},
    {0xe4f8, 0x14c, 2},
    {0xe4fa, 0x147, 2},
    {0xe4fc, 0x185, 2},
    {0xe4fe, 0x17a, 2},
    {0xe500, 0x187, 2},
    {0xe502, 0x17d, 2},
    {0xe504, 0x14f, 2},
    {0xe506, 0x148, 2},
    {0xe508, 0x139, 2},
    {0xe50a, 0x135, 2},
    {0xe50c, 0x12b, 2},
    {0xe50e, 0x129, 2},
    {0xe510, 0x11f, 2},
    {0xe512, 0x11d, 2},
    {0xe514, 0x115, 2},
    {0xe516, 0x114, 2},
    {0xe518, 0x10e, 2},
    {0xe51a, 0x10d, 2},
    {0xe51c, 0x10b, 2},
    {0xe51e, 0x10a, 2},
    {0xe520, 0x10b, 2},
    {0xe522, 0x10b, 2},
    {0xe524, 0x10f, 2},
    {0xe526, 0x10e, 2},
    {0xe528, 0x117, 2},
    {0xe52a, 0x116, 2},
    {0xe52c, 0x121, 2},
    {0xe52e, 0x120, 2},
    {0xe530, 0x12d, 2},
    {0xe532, 0x12b, 2},
    {0xe534, 0x13b, 2},
    {0xe536, 0x138, 2},
    {0xe538, 0x157, 2},
    {0xe53a, 0x150, 2},
    {0xe53c, 0x199, 2},
    {0xe53e, 0x18d, 2},
    {0xe540, 0x1a9, 2},
    {0xe542, 0x19c, 2},
    {0xe544, 0x162, 2},
    {0xe546, 0x15a, 2},
    {0xe548, 0x141, 2},
    {0xe54a, 0x13c, 2},
    {0xe54c, 0x133, 2},
    {0xe54e, 0x130, 2},
    {0xe550, 0x128, 2},
    {0xe552, 0x126, 2},
    {0xe554, 0x11f, 2},
    {0xe556, 0x11d, 2},
    {0xe558, 0x118, 2},
    {0xe55a, 0x117, 2},
    {0xe55c, 0x115, 2},
    {0xe55e, 0x114, 2},
    {0xe560, 0x116, 2},
    {0xe562, 0x114, 2},
    {0xe564, 0x119, 2},
    {0xe566, 0x118, 2},
    {0xe568, 0x120, 2},
    {0xe56a, 0x11f, 2},
    {0xe56c, 0x12a, 2},
    {0xe56e, 0x128, 2},
    {0xe570, 0x135, 2},
    {0xe572, 0x133, 2},
    {0xe574, 0x145, 2},
    {0xe576, 0x140, 2},
    {0xe578, 0x16d, 2},
    {0xe57a, 0x165, 2},
    {0xe57c, 0x1c1, 2},
    {0xe57e, 0x1b2, 2},
    {0xe580, 0x1e7, 2},
    {0xe582, 0x1d6, 2},
    {0xe584, 0x184, 2},
    {0xe586, 0x179, 2},
    {0xe588, 0x153, 2},
    {0xe58a, 0x14d, 2},
    {0xe58c, 0x13d, 2},
    {0xe58e, 0x139, 2},
    {0xe590, 0x133, 2},
    {0xe592, 0x130, 2},
    {0xe594, 0x12b, 2},
    {0xe596, 0x129, 2},
    {0xe598, 0x125, 2},
    {0xe59a, 0x123, 2},
    {0xe59c, 0x122, 2},
    {0xe59e, 0x121, 2},
    {0xe5a0, 0x123, 2},
    {0xe5a2, 0x121, 2},
    {0xe5a4, 0x126, 2},
    {0xe5a6, 0x124, 2},
    {0xe5a8, 0x12c, 2},
    {0xe5aa, 0x12a, 2},
    {0xe5ac, 0x135, 2},
    {0xe5ae, 0x132, 2},
    {0xe5b0, 0x140, 2},
    {0xe5b2, 0x13c, 2},
    {0xe5b4, 0x15b, 2},
    {0xe5b6, 0x154, 2},
    {0xe5b8, 0x193, 2},
    {0xe5ba, 0x188, 2},
    {0xe5bc, 0x207, 2},
    {0xe5be, 0x1f5, 2},
    {0xe5c0, 0x258, 2},
    {0xe5c2, 0x241, 2},
    {0xe5c4, 0x1c2, 2},
    {0xe5c6, 0x1b4, 2},
    {0xe5c8, 0x179, 2},
    {0xe5ca, 0x170, 2},
    {0xe5cc, 0x153, 2},
    {0xe5ce, 0x14d, 2},
    {0xe5d0, 0x141, 2},
    {0xe5d2, 0x13d, 2},
    {0xe5d4, 0x139, 2},
    {0xe5d6, 0x136, 2},
    {0xe5d8, 0x134, 2},
    {0xe5da, 0x131, 2},
    {0xe5dc, 0x132, 2},
    {0xe5de, 0x12f, 2},
    {0xe5e0, 0x132, 2},
    {0xe5e2, 0x130, 2},
    {0xe5e4, 0x134, 2},
    {0xe5e6, 0x132, 2},
    {0xe5e8, 0x13a, 2},
    {0xe5ea, 0x137, 2},
    {0xe5ec, 0x143, 2},
    {0xe5ee, 0x13f, 2},
    {0xe5f0, 0x159, 2},
    {0xe5f2, 0x153, 2},
    {0xe5f4, 0x185, 2},
    {0xe5f6, 0x17b, 2},
    {0xe5f8, 0x1da, 2},
    {0xe5fa, 0x1ca, 2},
    {0xe5fc, 0x28b, 2},
    {0xe5fe, 0x271, 2},
    
    // Final settings
    {0xd816, 0x3b5, 2},
    {0xd818, 0x9, 2},
    {0xd81a, 0x3e0, 2},
    {0xd81c, 0x18, 2},
    {0xd81e, 0x3, 2},
    {0xd820, 0x399, 2},
    {0xd02c, 0xdf2, 2},
    {0xd02e, 0x105c, 2},
    {0xd030, 0x188, 2},
    {0xd032, 0x289, 2},
    {0xd50a, 0x0, 1},
    {0xd960, 0x52, 1},
    {0xd961, 0x52, 1},
    {0xd962, 0x52, 1},
    {0xd963, 0x52, 1},
    {0xd96c, 0x44, 1},
    {0xd96d, 0x44, 1},
    {0xd96e, 0x44, 1},
    {0xd96f, 0x44, 1},
    {0xd600, 0x20, 1},
    {0x040c, 0x7ec, 2},
    {0x040e, 0x5f0, 2},
    {0x034c, 0x7ec, 2},
    {0x034e, 0x5f0, 2},
    {0x0342, 0x24b8, 2},
    {0x579a, 0xa, 2},
    {0x579c, 0x12a, 2},
    {0x57ac, 0x0, 2},
    {0x57ae, 0x81, 2},
    {0x57be, 0x0, 2},
    {0x57c0, 0x81, 2},
    {0x57d0, 0x0, 2},
    {0x57d2, 0x81, 2},
    {0x5324, 0x31, 2},
    {0x5326, 0x60, 2},
    {0xbca7, 0x8, 1},
    {0x5fcc, 0x1e, 1},
    {0x5fd7, 0x1e, 1},
    {0x5fe2, 0x1e, 1},
    {0x5fed, 0x1e, 1},
    {0x5ff8, 0x1e, 1},
    {0x6003, 0x1e, 1},
    {0x5d0b, 0x2, 1},
    {0x6f6d, 0x1, 1},
    {0x61c9, 0x68, 1},
    {0x5352, 0x3f, 2},
    {0x5356, 0x1c, 2},
    {0x5358, 0x3d, 2},
    {0x535c, 0xa6, 2},
    {0x6187, 0x1d, 1},
    {0x6189, 0x1d, 1},
    {0x618b, 0x1d, 1},
    {0x618d, 0x23, 1},
    {0x618f, 0x23, 1},
    {0x5414, 0x112, 2},
    {0xbca8, 0x0, 1},
    {0x5fcf, 0x28, 1},
    {0x5fda, 0x2d, 1},
    {0x5fe5, 0x2d, 1},
    {0x5ff0, 0x2d, 1},
    {0x5ffb, 0x2d, 1},
    {0x6006, 0x2d, 1},
    {0x616e, 0x4, 1},
    {0x616f, 0x4, 1},
    {0x6170, 0x4, 1},
    {0x6171, 0x6, 1},
    {0x6172, 0x6, 1},
    {0x6173, 0xc, 1},
    {0x6174, 0xc, 1},
    {0x6175, 0xc, 1},
    {0x6176, 0x10, 2},
    {0x6178, 0x1a, 2},
    {0x617a, 0x1a, 2},
    {0x617c, 0x27, 2},
    {0x617e, 0x27, 2},
    {0x6180, 0x44, 2},
    {0x6182, 0x44, 2},
    {0x6184, 0x44, 2},
    {0x5dfc, 0xa, 1},
    {0x5e00, 0xa, 1},
    {0x5e04, 0xa, 1},
    {0x5e08, 0xa, 1},
    {0x5dfd, 0xa, 1},
    {0x5e01, 0xa, 1},
    {0x5e05, 0xa, 1},
    {0x5e09, 0xa, 1},
    {0x5dfe, 0xa, 1},
    {0x5e02, 0xa, 1},
    {0x5e06, 0xa, 1},
    {0x5e0a, 0xa, 1},
    {0x5dff, 0xa, 1},
    {0x5e03, 0xa, 1},
    {0x5e07, 0xa, 1},
    {0x5e0b, 0xa, 1},
    {0x5dec, 0x12, 1},
    {0x5df0, 0x12, 1},
    {0x5df4, 0x21, 1},
    {0x5df8, 0x31, 1},
    {0x5ded, 0x12, 1},
    {0x5df1, 0x12, 1},
    {0x5df5, 0x21, 1},
    {0x5df9, 0x31, 1},
    {0x5dee, 0x12, 1},
    {0x5df2, 0x12, 1},
    {0x5df6, 0x21, 1},
    {0x5dfa, 0x31, 1},
    {0x5def, 0x12, 1},
    {0x5df3, 0x12, 1},
    {0x5df7, 0x21, 1},
    {0x5dfb, 0x31, 1},
    {0x5ddc, 0xd, 1},
    {0x5de0, 0xd, 1},
    {0x5de4, 0xd, 1},
    {0x5de8, 0xd, 1},
    {0x5ddd, 0xd, 1},
    {0x5de1, 0xd, 1},
    {0x5de5, 0xd, 1},
    {0x5de9, 0xd, 1},
    {0x5dde, 0xd, 1},
    {0x5de2, 0xd, 1},
    {0x5de6, 0xd, 1},
    {0x5dea, 0xd, 1},
    {0x5ddf, 0xd, 1},
    {0x5de3, 0xd, 1},
    {0x5de7, 0xd, 1},
    {0x5deb, 0xd, 1},
    {0x5dcc, 0x55, 1},
    {0x5dd0, 0x50, 1},
    {0x5dd4, 0x4b, 1},
    {0x5dd8, 0x4b, 1},
    {0x5dcd, 0x55, 1},
    {0x5dd1, 0x50, 1},
    {0x5dd5, 0x4b, 1},
    {0x5dd9, 0x4b, 1},
    {0x5dce, 0x55, 1},
    {0x5dd2, 0x50, 1},
    {0x5dd6, 0x4b, 1},
    {0x5dda, 0x4b, 1},
    {0x5dcf, 0x55, 1},
    {0x5dd3, 0x50, 1},
    {0x5dd7, 0x4b, 1},
    {0x5ddb, 0x4b, 1},
};

#define IMX501_REG_TABLE_SIZE (sizeof(imx501_reg_table) / sizeof(imx501_reg_table[0]))

#endif /* __REG_TABLE_H__ */