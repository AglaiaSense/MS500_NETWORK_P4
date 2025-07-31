
#include "as_tools.h"

bool is_plate_detection = false; // 车牌检测标志

int dnn_model_index = 0;
char license_plate[16] = "0";


Plate_result_t plate_result = {
    .x = 0,
    .y = 0,
    .w = 0,
    .h = 0,
    .image_count = 0,
    .score = 0.0f,
    .plate = "",
    .software_version = "100",
    .ai_model_version = "100"
};


imx500_reginfo_t* get_imx501_27m_1920x1080_crop_30fps_config(void) {
    static imx500_reginfo_t config[] = {
 // ---------------------------- 进行二次初始化  ----------------------------

        {0x0100, 0x00}, //  stop streaming

        {0x0136, 0x1B}, // input clock 27M
        {0x0137, 0x00}, // input clock 27M
        {0x0305, 0x02}, // IVT PREDIV
        {0x0306, 0x00}, // IVT MPY
        {0x0307, 0x9B}, // IVT MPY  2092.5M
        {0x030D, 0x02}, // IOP PREDIV
        {0x030E, 0x00}, // IOP MPY
        {0x030F, 0x2D}, // IOP MPY  607.5M
        // {0x030F, 0x4E},  // IOP MPY  1053M
        // {0x0340, 0x18},
        // {0x0341, 0x2C},
        // {0x0820, 0x10},  // MIPI RATE
        // {0x0821, 0x59},  // MIPI RATE
        {0x0820, 0x08}, // MIPI RATE   27 / 2 * 0x4E * 2
        {0x0821, 0x3A}, // MIPI RATE
        {0x0822, 0x00},
        {0x0823, 0x00},
        {0x3607, 0x01},
        {0x3E34, 0x00},
        {0x3E35, 0x01},
        {0x3E36, 0x01},
        {0x3E37, 0x00},
        {0x3E38, 0x01},
        {0x3E39, 0x01},
        {0x3E3A, 0x01},
        {0x3E3B, 0x00},

        {0x3F50, 0x00}, // normal streaming

        {0x3F56, 0x00},
        {0x3F57, 0xCA},
        {0x4BC0, 0x16},
        {0x7BA8, 0x00},
        {0x7BA9, 0x00},
        {0x886B, 0x00},
        // {0x0342, 0x23},
        // {0x0343, 0x40},
        {0x0112, 0x0A}, // 0x0A: 10bit, 0x08: 8bit
        {0x0113, 0x0A}, // 0x0A: 10bit, 0x08: 8bit
        {0x0114, 0x01}, // 0x01: 2lane, 0x00: 4lane
        {0x0202, 0x02}, // exposure  too long will affect the frame length, check 0x0350 register
        {0x0203, 0x94}, // exposure
        {0x0204, 0x03}, // again
        {0x0205, 0x56}, // again
        {0xBCF1, 0x00}, // EBD size
        // H2V2 Binning
        {0x0342, 0x44}, // Line Length: 17592
        {0x0343, 0xB8},
        // {0x0342, 0x24}, // Line Length: 9400
        // {0x0343, 0xB8},
        {0x3F56, 0x00},
        {0x3F57, 0xB8},
        // {0x0340, 0x07}, // Frame Length:1548   warn:ae
        // {0x0341, 0x0C},
        // {0x0340, 0x08}, // Frame Length:2212
        // {0x0341, 0xA4},
        {0x0900, 0x01}, // binning mode
        {0x0901, 0x22}, // 2x2 binning mode
        {0x034C, 0x07}, // Width: 1920
        {0x034D, 0x80},
        {0x034E, 0x04}, // Height: 1080
        {0x034F, 0x38},
        {0x0408, 0x00}, // X offset
        {0x0409, 0x36},
        {0x040a, 0x00}, // Y offset
        {0x040b, 0xDC},
        {0x040C, 0x07}, // Width: 1920
        {0x040D, 0x80},
        {0x040E, 0x04}, // Height: 1080
        {0x034F, 0x38},
        {0x0100, 0x01}, //  start streaming

        {0xffff, 0x00} // End of register list
    };
    return config;
}



void as_core_version(void) {
    printf("--------------------------\n");
    printf("AS_CODE Version 1.0.0\n");
}
