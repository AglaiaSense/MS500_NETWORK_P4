/*
 * 2024 Guangshi (Shanghai) CO LTD
 *
 * Li Xiang
 */

#ifndef UTILS_H_
#define UTILS_H_
#include <stdint.h>

typedef enum tagTensorType {
	TYPE_INPUT_TENSOR = 0,
	TYPE_OUTPUT_TENSOR
}TensorType;
	
namespace ai_utils {
	struct PacketHeader {
		uint8_t vc;
		uint8_t dataType;
		uint16_t wordCount;
		uint8_t ecc;
	};

	struct DnnHeader {
		uint8_t frameValid;
		uint8_t frameCount;
		uint16_t maxLineLen;
		uint16_t apParamSize;
		uint16_t networkId;
		uint8_t tensorType;
	};

	int extract_packet_header(const uint16_t* src, PacketHeader* packetHeader);
	int extract_dnn_header(const uint8_t* src, DnnHeader* dnnHeader);
}

#endif  // UTILS_H_
