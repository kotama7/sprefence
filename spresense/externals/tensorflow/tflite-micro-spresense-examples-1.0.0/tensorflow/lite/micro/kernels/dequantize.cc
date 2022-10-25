/* Copyright 2021 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#include "tensorflow/lite/kernels/internal/reference/dequantize.h"

#include "tensorflow/lite/c/builtin_op_data.h"
#include "tensorflow/lite/c/common.h"
#include "tensorflow/lite/kernels/internal/quantization_util.h"
#include "tensorflow/lite/kernels/internal/reference/quantize.h"
#include "tensorflow/lite/kernels/internal/reference/requantize.h"
#include "tensorflow/lite/kernels/internal/tensor_ctypes.h"
#include "tensorflow/lite/kernels/kernel_util.h"
#include "tensorflow/lite/micro/kernels/dequantize.h"
#include "tensorflow/lite/micro/kernels/kernel_util.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"

namespace tflite {

void* DequantizeInit(TfLiteContext* context, const char* buffer,
                     size_t length) {
  TFLITE_DCHECK(context->AllocatePersistentBuffer != nullptr);
  return context->AllocatePersistentBuffer(context, sizeof(DequantizeOpData));
}

static inline void DequantFloat16( const tflite::DequantizationParams& op_params,
                       const RuntimeShape& input_shape,
                       const TfLiteFloat16* input_data,
                       const RuntimeShape& output_shape, float* output_data) {
  int32_t zero_point = op_params.zero_point;
  const double scale = op_params.scale;
  const int flat_size = MatchingFlatSize(input_shape, output_shape);

  for (int i = 0; i < flat_size; i++) {
      const int sign_bit = input_data[i].data >> 15;
      const int exp_bit = (input_data[i].data >> 10) & 0x1f;
      const int mait_bit = (input_data[i].data & 0x03FF) << 13;
      float val = 0;
      unsigned int* val_bit = reinterpret_cast<unsigned int *>(&val);
      if (exp_bit == 0){
        *val_bit = (sign_bit << 31) | (*val_bit & (0x7F800000 | 0x007FFFFF));
        *val_bit = (mait_bit) | (*val_bit & (0x80000000 | 0x7F800000));
      }else{
        *val_bit = (sign_bit << 31) | (*val_bit & (0x7F800000 | 0x007FFFFF));
        *val_bit = ((exp_bit - 15 + 127) << 23) | (*val_bit & (0x80000000 | 0x007FFFFF));
        *val_bit = (mait_bit) | (*val_bit & (0x80000000 | 0x7F800000));
      }
      const float result = static_cast<float>(scale * (val - zero_point));
      output_data[i] = result;
  }
}

TfLiteStatus DequantizeEval(TfLiteContext* context, TfLiteNode* node) {
  TFLITE_DCHECK(node->user_data != nullptr);
  DequantizeOpData* data = static_cast<DequantizeOpData*>(node->user_data);

  const TfLiteEvalTensor* input = tflite::micro::GetEvalInput(context, node, 0);
  TfLiteEvalTensor* output = tflite::micro::GetEvalOutput(context, node, 0);

  if (output->type == kTfLiteFloat32) {
    switch (input->type) {
      case kTfLiteInt8:
        reference_ops::Dequantize(data->quantization_params,
                                  tflite::micro::GetTensorShape(input),
                                  tflite::micro::GetTensorData<int8_t>(input),
                                  tflite::micro::GetTensorShape(output),
                                  tflite::micro::GetTensorData<float>(output));
        break;
      case kTfLiteInt16:
        reference_ops::Dequantize(data->quantization_params,
                                  tflite::micro::GetTensorShape(input),
                                  tflite::micro::GetTensorData<int16_t>(input),
                                  tflite::micro::GetTensorShape(output),
                                  tflite::micro::GetTensorData<float>(output));
        break;
      case kTfLiteUInt8:
        reference_ops::Dequantize(data->quantization_params,
                                  tflite::micro::GetTensorShape(input),
                                  tflite::micro::GetTensorData<uint8_t>(input),
                                  tflite::micro::GetTensorShape(output),
                                  tflite::micro::GetTensorData<float>(output));
        break;
      case kTfLiteFloat16:
        DequantFloat16(data->quantization_params,
                                  tflite::micro::GetTensorShape(input),
                                  tflite::micro::GetTensorData<TfLiteFloat16>(input),
                                  tflite::micro::GetTensorShape(output),
                                  tflite::micro::GetTensorData<float>(output));
        break;
      default:
        MicroPrintf("Input %s, output %s not supported.",
                    TfLiteTypeGetName(input->type),
                    TfLiteTypeGetName(output->type));
        return kTfLiteError;
    }
  } else {
    MicroPrintf("Input %s, output %s not supported.",
                TfLiteTypeGetName(input->type),
                TfLiteTypeGetName(output->type));
    return kTfLiteError;
  }

  return kTfLiteOk;
}

TfLiteRegistration Register_DEQUANTIZE() {
  return {/*init=*/DequantizeInit,
          /*free=*/nullptr,
          /*prepare=*/DequantizePrepare,
          /*invoke=*/DequantizeEval,
          /*profiling_string=*/nullptr,
          /*builtin_code=*/0,
          /*custom_name=*/nullptr,
          /*version=*/0};
}

}  // namespace tflite
