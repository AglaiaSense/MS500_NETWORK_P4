/*
 * 2024 Guangshi (Shanghai) CO LTD
 *
 * Li Xiang
 */

#ifndef __IMX501_MACRO_H__
#define __IMX501_MACRO_H__

#include <stdint.h>



#define SC_DNN_MAX_NETWORK_ID       (0x999999)  /* 最大网络ID（十六进制） */
#define SC_DNN_MAX_NETWORK_ID_DEC (999999)  /* 最大网络ID（十进制） */

#define MAX_OUTPUT_TENSOR_NUM (30)  /* 最大输出张量数量 */
#define MAX_DNN_HEADER_SIZE (4096)  /* 最大DNN头部数据大小 */



#define DNN_INPUT_FORMAT_RGB        (0)  /* 输入格式：RGB */
#define DNN_INPUT_FORMAT_Y          (1)  /* 输入格式：Y（灰度） */
#define DNN_INPUT_FORMAT_YUV444     (2)  /* 输入格式：YUV444 */
#define DNN_INPUT_FORMAT_YUV420     (3)  /* 输入格式：YUV420 */
#define DNN_INPUT_FORMAT_BGR        (4)  /* 输入格式：BGR */
#define DNN_INPUT_FORMAT_BAYER_RGB  (5)  /* 输入格式：Bayer RGB */


/* download firmware */
static const char *IMAGE_FILE_LOADER = "/spiffs/firmware/loader.fpk";
static const char *IMAGE_FILE_MAINFW = "/spiffs/firmware/firmware.fpk";

// 读写fpk的内存大小
#define IMAGE_FILE_MEMORY_SIZE      (40960)     // 40 * 1024


/* imx501 configuration */

#define IMX501_ID_REG_ADDR          (0xF780)         /*!< Register addresses of the id register */
#define IMX501_VERSION_REG_ADDR     (0xF72D)         /*!< Register addresses of the version register */
#define IMX501_STREAM_REG_ADDR      (0x0100)         /*!< Register addresses of the stream register */

#define DEF_VAL_NOTIMAGING_ONLY_MODE_FW         (0x00)          /* not image only */
#define DEF_VAL_IMAGING_ONLY_MODE_FW            (0x01)          /* image only */
#define REG_ADDR_IMAGING_ONLY_MODE_FW           (0xA700)

#define REG_ADDR_EXCK_FREQ_C01                  (0x0136)
#define REG_ADDR_IVT_PREPLLCK_DIV_C03           (0x0305)
#define REG_ADDR_IVT_PLL_MPY_C03                (0x0306)
#define REG_ADDR_IOP_PREPLLCK_DIV_C03           (0x030D)
#define REG_ADDR_IOP_PLL_MPY_C03                (0x030E)

#define REG_ADDR_CSI_LANE_MODE                  (0x0114)
#define REG_SIZE_CSI_LANE_MODE                  (1)
#define REG_ADDR_X_OUT_SIZE                     (0x034C)
#define REG_SIZE_X_OUT_SIZE                     (2)
#define REG_ADDR_Y_OUT_SIZE                     (0x034E)
#define REG_SIZE_Y_OUT_SIZE                     (2)

#define REG_ADDR_SCALE_M_EXT                    (0x3264)
#define REG_SIZE_SCALE_M_EXT                    (2)
#define REG_ADDR_SCALE_MODE_EXT                 (0x3261)
#define DEF_VAL_SCALE_MODE_EXT                  (0x01)
#define REG_SIZE_SCALE_MODE_EXT                 (1)
#define REG_ADDR_SCALE_MODE                     (0x0401)
#define DEF_VAL_SCALE_MODE                      (0x02)
#define REG_SIZE_SCALE_MODE                     (1)

#define REG_ADDR_SCALE_M                        (0x0404)
#define REG_SIZE_SCALE_M                        (2)
#define REG_ADDR_DIG_CROP_IMAGE_WIDTH           (0x040C)
#define REG_ADDR_DIG_CROP_IMAGE_HEIGHT          (0x040E)
#define REG_SIZE_DIG_CROP_IMAGE_WIDTH           (2)
#define REG_SIZE_DIG_CROP_IMAGE_HEIGHT          (2)

#define REG_ADDR_REQ_LINK_BIT_RATE_MBPS_C08     (0x0820)
#define REG_SIZE_REQ_LINK_BIT_RATE_MBPS_C08     (4)

#define REG_ADDR_FLASH_TYPE                     (0xD005)

#define REG_ADDR_LEV_PL_GAIN_VALUE              (0xD600)

#define REG_SIZE_IMAGING_ONLY_MODE_FW           (1)
#define REG_SIZE_IVT_PREPLLCK_DIV_C03           (1)
#define REG_SIZE_IOP_PREPLLCK_DIV_C03           (1)
#define REG_SIZE_IVT_PLL_MPY_C03                (2)
#define REG_SIZE_IOP_PLL_MPY_C03                (2)
#define REG_SIZE_EXCK_FREQ_C01                  (2)
#define REG_SIZE_FLASH_TYPE                     (1)


#define DEF_VAL_DD_X_OUT_SIZE_V2H2              (2028)
#define DEF_VAL_DD_Y_OUT_SIZE_V2H2              (1520)
#define DEF_VAL_DD_X_OUT_SIZE_FULLSIZE          (4056)
#define DEF_VAL_DD_Y_OUT_SIZE_FULLSIZE          (3040)

#define REG_ADDR_LINE_LENGTH_PCK_C03            (0x0342)
#define REG_SIZE_LINE_LENGTH_PCK_C03            (2)
#define TOTAL_NUM_OF_IVTPX_CH                   (4)
#define IVTPXCK_SYCK_DIV                        (2)
#define IVTPXCK_PXCK_DIV                        (5)
#define IVTPXCK_CLOCK_DIVISION_RATIO            (IVTPXCK_SYCK_DIV * IVTPXCK_PXCK_DIV)
#define REG_ADDR_LINE_LENGTH_INCK_M0F           (0x3F56)
#define REG_SIZE_LINE_LENGTH_INCK_M0F           (2)
#define FRM_LENGTH_LINES_SIZE_MAX               (65535)
#define PIXEL_RATE_FOR_CALC_FRM_RATE            ((210000000 - 500000) * TOTAL_NUM_OF_IVTPX_CH)
#define DEF_VAL_DNN_MAX_RUN_TIME                (33)

#define REG_ADDR_FRM_LENGTH_LINES_C03           (0x0340)
#define REG_SIZE_FRM_LENGTH_LINES_C03           (2)
#define REG_ADDR_FLL_LSHIFT_M02                 (0x3210)
#define REG_SIZE_FLL_LSHIFT_M02                 (1)
#define REG_ADDR_DNN_MAX_RUN_TIME               (0xD038)
#define REG_SIZE_DNN_MAX_RUN_TIME               (2)


#define REG_ADDR_DD_CH06_X_OUT_SIZE             (0x3054)
#define REG_ADDR_DD_CH07_X_OUT_SIZE             (0x3056)
#define REG_ADDR_DD_CH08_X_OUT_SIZE             (0x3058)
#define REG_ADDR_DD_CH09_X_OUT_SIZE             (0x305A)
#define REG_ADDR_DD_CH06_Y_OUT_SIZE             (0x305C)
#define REG_ADDR_DD_CH07_Y_OUT_SIZE             (0x305E)
#define REG_ADDR_DD_CH08_Y_OUT_SIZE             (0x3060)
#define REG_ADDR_DD_CH09_Y_OUT_SIZE             (0x3062)
#define REG_SIZE_DD_CH06_X_OUT_SIZE             (2)
#define REG_SIZE_DD_CH07_X_OUT_SIZE             (2)
#define REG_SIZE_DD_CH08_X_OUT_SIZE             (2)
#define REG_SIZE_DD_CH09_X_OUT_SIZE             (2)
#define REG_SIZE_DD_CH06_Y_OUT_SIZE             (2)
#define REG_SIZE_DD_CH07_Y_OUT_SIZE             (2)
#define REG_SIZE_DD_CH08_Y_OUT_SIZE             (2)
#define REG_SIZE_DD_CH09_Y_OUT_SIZE             (2)

#define DEF_VAL_SCALE_M                         (16)
#define DEF_VAL_SCALE_M_EXT                     (DEF_VAL_SCALE_M)
#define RAW_IMAGE_WIDTH_MIN                     (176)
#define RAW_IMAGE_HEIGHT_MIN                    (144)

#define REG_ADDR_BINNING_MODE                   (0x0900)
#define REG_ADDR_BINNING_TYPE                   (0x0901)
#define DEF_VAL_BINNING_MODE                    (0x01)
#define DEF_VAL_BINNING_TYPE_V2H2               (0x22) 
#define REG_SIZE_BINNING_MODE                   (1)
#define REG_SIZE_BINNING_TYPE                   (1)

#define REG_ADDR_BINNING_WEIGHTING              (0x0902)
#define REG_ADDR_SUB_WEIGHTING                  (0x3250)
#define DEF_VAL_SUB_WEIGHTING_BAYER             (0x03)
#define DEF_VAL_BINNING_WEIGHTING_BAYER         (0x02)
#define REG_SIZE_BINNING_WEIGHTING              (1)
#define REG_SIZE_SUB_WEIGHTING                  (1)


#define IMX500_HW_ALIGN                         (16)
#define IMX500_HW_LIMIT_LINE_PIX                (2560)


#define REG_OFST_DNN_INPUT_NORM_CH              (0x08)
#define REG_OFST_DNN_INPUT_NORM                 (0x18)
#define REG_OFST_DNN_NORM_CLIP                  (0x0C)
#define REG_OFST_DNN_YCMTRX                     (0x24)
#define REG_OFST_LEV_PL_GAIN                    (0x01)
#define REG_OFST0_LEV_PL_NORM_YM_YADD           (0x00)
#define REG_OFST1_LEV_PL_NORM_YM_YADD           (REG_OFST0_LEV_PL_NORM_YM_YADD + 0x1A)
#define REG_OFST2_LEV_PL_NORM_YM_YADD           (REG_OFST1_LEV_PL_NORM_YM_YADD + 0x0C)
#define REG_OFST0_LEV_PL_NORM_YM_YSFT           (0x00)
#define REG_OFST1_LEV_PL_NORM_YM_YSFT           (REG_OFST0_LEV_PL_NORM_YM_YSFT + 0x1A)
#define REG_OFST2_LEV_PL_NORM_YM_YSFT           (REG_OFST1_LEV_PL_NORM_YM_YSFT + 0x0C)
#define REG_OFST0_LEV_PL_NORM_CB_YADD           (0x00)
#define REG_OFST1_LEV_PL_NORM_CB_YADD           (REG_OFST0_LEV_PL_NORM_CB_YADD + 0x16)
#define REG_OFST2_LEV_PL_NORM_CB_YADD           (REG_OFST1_LEV_PL_NORM_CB_YADD + 0x0C)
#define REG_OFST0_LEV_PL_NORM_CB_YSFT           (0x00)
#define REG_OFST1_LEV_PL_NORM_CB_YSFT           (REG_OFST0_LEV_PL_NORM_CR_YSFT + 0x17)
#define REG_OFST2_LEV_PL_NORM_CB_YSFT           (REG_OFST1_LEV_PL_NORM_CR_YSFT + 0x0C)
#define REG_OFST0_LEV_PL_NORM_CR_YADD           (0x00)
#define REG_OFST1_LEV_PL_NORM_CR_YADD           (REG_OFST0_LEV_PL_NORM_CR_YADD + 0x12)
#define REG_OFST2_LEV_PL_NORM_CR_YADD           (REG_OFST1_LEV_PL_NORM_CR_YADD + 0x0C)
#define REG_OFST0_LEV_PL_NORM_CR_YSFT           (0x00)
#define REG_OFST1_LEV_PL_NORM_CR_YSFT           (REG_OFST0_LEV_PL_NORM_CR_YSFT + 0x13)
#define REG_OFST2_LEV_PL_NORM_CR_YSFT           (REG_OFST1_LEV_PL_NORM_CR_YSFT + 0x0C)
#define REG_OFST_ROT_DNN_NORM_CH                (0x08)
#define REG_OFST_ROT_DNN_NORM_DNN               (0x20)


#define REG_ADDR_DNN_YCMTRX_K00                 (0xD76C)
#define REG_ADDR_DNN_YCMTRX_K01                 (0xD76E)
#define REG_ADDR_DNN_YCMTRX_K02                 (0xD770)
#define REG_ADDR_DNN_YCMTRX_K03                 (0xD772)
#define REG_ADDR_DNN_YCMTRX_K10                 (0xD774)
#define REG_ADDR_DNN_YCMTRX_K11                 (0xD776)
#define REG_ADDR_DNN_YCMTRX_K12                 (0xD778)
#define REG_ADDR_DNN_YCMTRX_K13                 (0xD77A)
#define REG_ADDR_DNN_YCMTRX_K20                 (0xD77C)
#define REG_ADDR_DNN_YCMTRX_K21                 (0xD77E)
#define REG_ADDR_DNN_YCMTRX_K22                 (0xD780)
#define REG_ADDR_DNN_YCMTRX_K23                 (0xD782)
#define REG_ADDR_DNN_YCMTRX_Y_CLIP              (0xD784)
#define REG_ADDR_DNN_YCMTRX_CB_CLIP             (0xD788)
#define REG_ADDR_DNN_YCMTRX_CR_CLIP             (0xD78C)
#define REG_ADDR_DNN_INPUT_NORM                 (0xD708)
#define REG_ADDR_DNN_INPUT_NORM_SHIFT           (0xD70A)
#define REG_ADDR_DNN_INPUT_NORM_CLIP_MAX        (0xD70C)
#define REG_ADDR_DNN_INPUT_NORM_CLIP_MIN        (0xD70E)
#define REG_ADDR_DNN_INPUT_FORMAT_BASE          (0xD750)

#define REG_SIZE_DNN_YCMTRX_K00                 (2)
#define REG_SIZE_DNN_YCMTRX_K01                 (2)
#define REG_SIZE_DNN_YCMTRX_K02                 (2)
#define REG_SIZE_DNN_YCMTRX_K03                 (2)
#define REG_SIZE_DNN_YCMTRX_K10                 (2)
#define REG_SIZE_DNN_YCMTRX_K11                 (2)
#define REG_SIZE_DNN_YCMTRX_K12                 (2)
#define REG_SIZE_DNN_YCMTRX_K13                 (2)
#define REG_SIZE_DNN_YCMTRX_K20                 (2)
#define REG_SIZE_DNN_YCMTRX_K21                 (2)
#define REG_SIZE_DNN_YCMTRX_K22                 (2)
#define REG_SIZE_DNN_YCMTRX_K23                 (2)
#define REG_SIZE_DNN_YCMTRX_Y_CLIP              (4)
#define REG_SIZE_DNN_YCMTRX_CB_CLIP             (4)
#define REG_SIZE_DNN_YCMTRX_CR_CLIP             (4)
#define REG_SIZE_ROT_DNN_NORM                   (2)
#define REG_SIZE_ROT_DNN_NORM_SHIFT             (1)
#define REG_SIZE_ROT_DNN_NORM_CLIP_MAX          (2)
#define REG_SIZE_ROT_DNN_NORM_CLIP_MIN          (2)
#define REG_SIZE_DNN_INPUT_NORM                 (2)
#define REG_SIZE_DNN_INPUT_NORM_SHIFT           (1)
#define REG_SIZE_DNN_INPUT_NORM_CLIP_MAX        (2)
#define REG_SIZE_DNN_INPUT_NORM_CLIP_MIN        (2)
#define REG_SIZE_DNN_INPUT_FORMAT               (1)


#define REG_ADDR_DNN_NORM_YM_CLIP               (0xD7D8)
#define REG_ADDR_DNN_NORM_CB_CLIP               (0xD7DC)
#define REG_ADDR_DNN_NORM_CR_CLIP               (0xD7E0)
#define REG_SIZE_DNN_NORM_YM_CLIP               (4)
#define REG_SIZE_DNN_NORM_CB_CLIP               (4)
#define REG_SIZE_DNN_NORM_CR_CLIP               (4)

#define REG_ADDR_LEV_PL_GAIN_VALUE              (0xD600)
#define REG_ADDR_LEV_PL_NORM_YM_YSFT            (0xD629)
#define REG_ADDR_LEV_PL_NORM_YM_YADD            (0xD62A)
#define REG_ADDR_LEV_PL_NORM_CB_YSFT            (0xD630)
#define REG_ADDR_LEV_PL_NORM_CB_YADD            (0xD632)
#define REG_ADDR_LEV_PL_NORM_CR_YSFT            (0xD638)
#define REG_ADDR_LEV_PL_NORM_CR_YADD            (0xD63A)
#define REG_SIZE_LEV_PL_GAIN_VALUE              (1)
#define REG_SIZE_LEV_PL_NORM_YM_YSFT            (1)
#define REG_SIZE_LEV_PL_NORM_YM_YADD            (2)
#define REG_SIZE_LEV_PL_NORM_CB_YSFT            (1)
#define REG_SIZE_LEV_PL_NORM_CB_YADD            (2)
#define REG_SIZE_LEV_PL_NORM_CR_YSFT            (1)
#define REG_SIZE_LEV_PL_NORM_CR_YADD            (2)

#define REG_ADDR_ROT_DNN_NORM                   (0xD684)
#define REG_ADDR_ROT_DNN_NORM_SHIFT             (0xD686)
#define REG_ADDR_ROT_DNN_NORM_CLIP_MAX          (0xD688)
#define REG_ADDR_ROT_DNN_NORM_CLIP_MIN          (0xD68A)

#define IMX500SF_REG_ADDR_DD_STRM_MODE_SEL  (0xD100)
#define IMX500SF_REG_SIZE_DD_STRM_MODE_SEL  (1)
#define DD_STRM_MODE_DNN_ENABLED            (0x4)

#define REG_ADDR_MODE_SEL                       (0x0100)
#define STREAM_START                            (0x01)
#define STREAM_STANDBY                           (0x00)
#define REG_SIZE_MODE_SEL                       (1)


#define YCMTRX_KX0_2_DEC_SHT            (10)        /* decimal point places between bit10 and bit9. */
#define YCMTRX_KX0_2_SIGNED_SHT         (11)        /* bit11 is sign bit. */
#define YCMTRX_KX0_2_MASK               (0x0FFF)
#define YCMTRX_KX3_DEC_SHT              (4)         /* decimal point places between bit4 and bit3. */
#define YCMTRX_KX3_SIGNED_SHT           (12)        /* bit12 is sign bit. */
#define YCMTRX_KX3_MASK                 (0x1FFF)

#define FREQ_1MHZ           (1000 * 1000)
#define INCK_12MHZ          (12 * FREQ_1MHZ)
#define INCK_23MHZ          (23 * FREQ_1MHZ)
#define INCK_27MHZ          (27 * FREQ_1MHZ)
#define TARGET_PLL_FREQ     (2100 * FREQ_1MHZ)


/* IMX 500 Register & Setting Param */
#define IMX500SF_REG_ADDR_DD_ST_TRANS_CMD                   (0xD000)
#define IMX500SF_REG_SIZE_DD_ST_TRANS_CMD                   (1)
#define IMX500SF_REG_LOADER_LOAD_ST_TRANS_CMD               (0x00)
#define IMX500SF_REG_MAINFW_LOAD_ST_TRANS_CMD               (0x01)
#define IMX500SF_REG_WO_NW_TO_W_NW_ST_TRANS_CMD             (0x02)
#define IMX500SF_REG_W_NW_TO_WO_NW_ST_TRANS_CMD             (0x03)

#define IMX500SF_REG_ADDR_DD_UPDATE_CMD                     (0xD001)
#define IMX500SF_REG_SIZE_DD_UPDATE_CMD                     (1)
#define IMX500SF_REG_DD_UPDATE_CMD_UPDATE_L2MEM             (0x00)
#define IMX500SF_REG_DD_UPDATE_CMD_UPDATE_FLASH             (0x01)

#define IMX500SF_REG_ADDR_DD_LOAD_MODE                   (0xD002)
#define IMX500SF_REG_SIZE_DD_LOAD_MODE                   (1)
#define IMX500SF_REG_FROMAP_DD_LOAD_MODE                 (0x00)
#define IMX500SF_REG_FROMFLASH_DD_LOAD_MODE              (0x01)

#define IMX500SF_REG_ADDR_DD_IMAGE_TYPE                  (0xD003)
#define IMX500SF_REG_SIZE_DD_IMAGE_TYPE                  (1)
#define IMX500SF_REG_LOADER_DD_IMAGE_TYPE                 (0x00)
#define IMX500SF_REG_MAIN_DD_IMAGE_TYPE                   (0x01)
#define IMX500SF_REG_NW_DD_IMAGE_TYPE                     (0x02)

#define IMX500SF_REG_ADDR_DD_DOWNLOAD_DIV_NUM            (0xD004)
#define IMX500SF_REG_SIZE_DD_DOWNLOAD_DIV_NUM            (1)

#define IMX500SF_REG_ADDR_DD_DOWNLOAD_FILE_SIZE          (0xD008)
#define IMX500SF_REG_SIZE_DD_DOWNLOAD_FILE_SIZE          (4)

#define IMX500SF_REG_ADDR_DD_UPDATE_CMD                  (0xD001)
#define IMX500SF_REG_SIZE_DD_UPDATE_CMD                  (1)

#define IMX500SF_REG_ADDR_DD_FLASH_ADDR                  (0xD00C)
#define IMX500SF_REG_SIZE_DD_FLASH_ADDR                  (4)

#define IMX500SF_REG_ADDR_DD_CMD_INT                     (0x3080)
#define IMX500SF_REG_SIZE_DD_CMD_INT                     (1)
#define IMX500SF_REG_TRANS_CMD_DD_CMD_INT                (0)
#define IMX500SF_REG_UPDATE_CMD_DD_CMD_INT               (1)

#define IMX500SF_REG_ADDR_DD_CMD_REPLY_STS               (0xD014)
#define IMX500SF_REG_SIZE_DD_CMD_REPLY_STS               (1)
#define IMX500SF_REG_DD_CMD_RPLY_STS_TRNS_READY          (0x00)
#define IMX500SF_REG_DD_CMD_RPLY_STS_TRNS_DONE           (0x01)
#define IMX500SF_REG_DD_CMD_RPLY_STS_UPDATE_READY        (0x10)
#define IMX500SF_REG_DD_CMD_RPLY_STS_UPDATE_DONE         (0x11)

#define IMX500SF_REG_ADDR_DD_DOWNLOAD_STS               (0xD015)
#define IMX500SF_REG_SIZE_DD_DOWNLOAD_STS               (1)
#define IMX500SF_REG_READY_DD_DOWNLOAD_STS              (0x00)
#define IMX500SF_REG_DOWNLOADING_DD_DOWNLOAD_STS        (0x01)

#define IMX500SF_REG_ADDR_DD_REF_STS_REG                 (0xD010)
#define IMX500SF_REG_SIZE_DD_REF_STS_REG                 (1)

#define IMX500SF_REG_ADDR_DD_SYS_STATE                   (0xD02A)
#define IMX500SF_REG_SIZE_DD_SYS_STATE                   (1)
#define IMX500SF_REG_DD_SYS_STATE_STANDBY_WONET          (0)
#define IMX500SF_REG_DD_SYS_STATE_STANDBY_WNET           (2)

#define IMX500SF_REG_ADDR_DD_REF_STS_REG                 (0xD010)
#define IMX500SF_REG_SIZE_DD_REF_STS_REG                 (1)

#define IMX500SF_SPI_ONE_TRANS_SIZE     (4096)

#define IMX500SF_POLLING_TIMEOUT        (0xFFFFFFFF)
#define IMX500SF_NW_TRANS_SIZE_UNIT     (1024*1024)

 
#endif /* __IMX501_MACRO_H__ */
