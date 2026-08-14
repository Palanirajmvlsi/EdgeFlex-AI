#ifndef TINYMLP_MODEL_H
#define TINYMLP_MODEL_H

#include "edgeflex_model.h"

#define TINYMLP_INPUT_SIZE  (8u)
#define TINYMLP_OUTPUT_SIZE (4u)

extern const edgeflex_model_t g_tinymlp_model;
extern const float g_tinymlp_sample_input[TINYMLP_INPUT_SIZE];

#endif /* TINYMLP_MODEL_H */
