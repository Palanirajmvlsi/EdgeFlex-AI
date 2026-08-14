#include "edgeflex_model.h"

edgeflex_model_status_t edgeflex_model_validate(const edgeflex_model_t *model)
{
    if (model == NULL || model->layers == NULL) {
        return EDGEFLEX_MODEL_ERR_NULL;
    }
    if (model->layer_count == 0) {
        return EDGEFLEX_MODEL_ERR_NO_LAYERS;
    }
    if (model->layer_count > EDGEFLEX_MODEL_MAX_LAYERS) {
        return EDGEFLEX_MODEL_ERR_TOO_MANY_LAYERS;
    }

    if (model->layers[0].input_size != model->input_size) {
        return EDGEFLEX_MODEL_ERR_SHAPE_MISMATCH;
    }
    if (model->layers[model->layer_count - 1].output_size != model->output_size) {
        return EDGEFLEX_MODEL_ERR_SHAPE_MISMATCH;
    }

    for (uint8_t i = 0; i < model->layer_count; i++) {
        const edgeflex_layer_t *l = &model->layers[i];

        if (l->input_size == 0 || l->output_size == 0) {
            return EDGEFLEX_MODEL_ERR_ZERO_DIM;
        }

        if (i > 0 && model->layers[i - 1].output_size != l->input_size) {
            return EDGEFLEX_MODEL_ERR_SHAPE_MISMATCH;
        }

        if ((l->op == EDGEFLEX_OP_DENSE || l->op == EDGEFLEX_OP_DENSE_RELU) &&
            (l->weights == NULL || l->biases == NULL)) {
            return EDGEFLEX_MODEL_ERR_MISSING_WEIGHTS;
        }
    }

    return EDGEFLEX_MODEL_OK;
}

size_t edgeflex_model_layer_workspace_bytes(const edgeflex_model_t *model, uint8_t idx)
{
    if (model == NULL || model->layers == NULL || idx >= model->layer_count) {
        return 0;
    }
    return (size_t)model->layers[idx].output_size * sizeof(float);
}

const char *edgeflex_model_status_str(edgeflex_model_status_t status)
{
    switch (status) {
        case EDGEFLEX_MODEL_OK:                   return "OK";
        case EDGEFLEX_MODEL_ERR_NULL:              return "ERR_NULL";
        case EDGEFLEX_MODEL_ERR_NO_LAYERS:         return "ERR_NO_LAYERS";
        case EDGEFLEX_MODEL_ERR_TOO_MANY_LAYERS:   return "ERR_TOO_MANY_LAYERS";
        case EDGEFLEX_MODEL_ERR_SHAPE_MISMATCH:    return "ERR_SHAPE_MISMATCH";
        case EDGEFLEX_MODEL_ERR_MISSING_WEIGHTS:   return "ERR_MISSING_WEIGHTS";
        case EDGEFLEX_MODEL_ERR_ZERO_DIM:          return "ERR_ZERO_DIM";
        default:                                   return "ERR_UNKNOWN";
    }
}
