/*
 * 2024 Guangshi (Shanghai) CO LTD
 *
 * Li Xiang
 */


#include <stdint.h>
#include <iostream>
#include "ai_utils.h"

#define bytes_to_uint16(MSB,LSB) (((uint16_t) ((unsigned char) MSB)) & 255)<<8 | (((unsigned char) LSB)&255) 
#define bytes_to_int16(MSB,LSB) (((int16_t) ((unsigned char) MSB)) & 255)<<8 | (((unsigned char) LSB)&255) 
#define extract_byte(MSB,LSB) (((((unsigned char) MSB)<<6) & 0xc0) | ((((unsigned char) LSB) & 0xfc) >> 2))

using namespace ai_utils;

/* Extract Packet Header Data */
int ai_utils::extract_packet_header(const uint16_t* src, PacketHeader* packetHeader) {
	packetHeader->vc = (*src) & 0xc0;
	packetHeader->dataType = (*src) & 0x3f;
	packetHeader->wordCount = (*(src + 2) << 8 | *(src + 1)) * 8 / 10;
	return 0;
}

/* Extract Dnn Header Data */
int ai_utils::extract_dnn_header(const uint8_t* src, DnnHeader* dnnHeader) {

	if(src == NULL) {
	    printf("[UTIL][extract_dnn_header][ERROR] src is NULL\n");
	    return -1;	
	}

	if(dnnHeader == NULL) {
	    printf("[UTIL][extract_dnn_header][ERROR] dnnHeader is NULL\n");
	    return -1;	
	}

	dnnHeader->frameValid = *(src++);
	dnnHeader->frameCount = *(src++);

	dnnHeader->maxLineLen =  (*(src+1) << 8 |*(src));
        src+=2;
	dnnHeader->apParamSize = (*(src+1) << 8 |*(src));
        src+=2;
	dnnHeader->networkId = (*(src+1) << 8 |*(src));
        src+=2;
	dnnHeader->tensorType = *(src++);
	
	//printf("[UTIL] dnn_header:\n");
	//printf("[UTIL] frameValid=%d, frameCount=%d, mll=%d, apParamSize=%d, nwId=%d, tensorType=%d\n",
	//				dnnHeader->frameValid,	dnnHeader->frameCount, dnnHeader->maxLineLen,
	//				dnnHeader->apParamSize, dnnHeader->networkId, dnnHeader->tensorType);
	return 0;
}


