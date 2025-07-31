#include "bsp_inference.h"
#include "as_tools.h"
#include <string.h>

extern int decode_yolo(const void *data);

extern int decode_plate(const void *data);

int decode_dnn_output(const void *data) {

    if (dnn_model_index == 0) {
        decode_yolo(data);
    } else if (dnn_model_index == 1) {

        decode_plate(data);
    }

    return 0;

    return 0;
}
