/* Copyright 2019 The TensorFlow Authors. All Rights Reserved.

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

#include <string.h>

#include "face_detect.h"

#include "face_image_provider.h"
#include "face_model_settings.h"
#include "model_data.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/system_setup.h"
#include "tensorflow/lite/schema/schema_generated.h"

// Globals, used for compatibility with Arduino-style sketches.
namespace {
tflite::ErrorReporter* f_error_reporter = nullptr;
const tflite::Model* f_model = nullptr;
tflite::MicroInterpreter* f_interpreter = nullptr;
TfLiteTensor* f_input = nullptr;

// In order to use optimized tensorflow lite kernels, a signed int8_t quantized
// model is preferred over the legacy unsigned model format. This means that
// throughout this project, input images must be converted from unisgned to
// signed format. The easiest and quickest way to convert from unsigned to
// signed 8-bit integers is to subtract 128 from the unsigned value to get a
// signed value.

// An area of memory to use for input, output, and intermediate arrays.
constexpr int f_kTensorArenaSize = 1024 * 1024 * 2;
static uint8_t f_tensor_arena[f_kTensorArenaSize];
}  // namespace

constexpr size_t ko0Size = sizeof(float) * 896 * 16;
constexpr size_t ko1Size = sizeof(float) * 896;

// The name of this function is important for Arduino compatibility.
void FaceSetup() {
  tflite::InitializeTarget();

  // Set up logging. Google style is to avoid globals or statics because of
  // lifetime uncertainty, but since this has a trivial destructor it's okay.
  // NOLINTNEXTLINE(runtime-global-variables)
  static tflite::MicroErrorReporter f_micro_error_reporter;
  f_error_reporter = &f_micro_error_reporter;

  // Map the model into a usable data structure. This doesn't involve any
  // copying or parsing, it's a very lightweight operation.
  f_model = tflite::GetModel(face_detection_short_range_tflite);
  if (f_model->version() != TFLITE_SCHEMA_VERSION) {
    TF_LITE_REPORT_ERROR(f_error_reporter,
                         "Model provided is schema version %d not equal "
                         "to supported version %d.",
                         f_model->version(), TFLITE_SCHEMA_VERSION);
    return;
  }

  // Pull in only the operation implementations we need.
  // This relies on a complete list of all the ops needed by this graph.
  // An easier approach is to just use the AllOpsResolver, but this will
  // incur some penalty in code space for op implementations that are not
  // needed by this graph.
  //
  // tflite::AllOpsResolver resolver;
  // NOLINTNEXTLINE(runtime-global-variables)
  static tflite::MicroMutableOpResolver<10> micro_op_resolver;
  micro_op_resolver.AddAveragePool2D();
  micro_op_resolver.AddConv2D();
  micro_op_resolver.AddDepthwiseConv2D();
  micro_op_resolver.AddReshape();
  micro_op_resolver.AddAdd();
  micro_op_resolver.AddRelu();
  micro_op_resolver.AddPad();
  micro_op_resolver.AddMaxPool2D();
  micro_op_resolver.AddConcatenation();
  micro_op_resolver.AddDequantize();

  // Build an interpreter to run the model with.
  // NOLINTNEXTLINE(runtime-global-variables)
  static tflite::MicroInterpreter static_interpreter(
      f_model, micro_op_resolver, f_tensor_arena, f_kTensorArenaSize, f_error_reporter);
  f_interpreter = &static_interpreter;

  // Allocate memory from the tensor_arena for the model's tensors.
  TfLiteStatus allocate_status = f_interpreter->AllocateTensors();
  if (allocate_status != kTfLiteOk) {
    TF_LITE_REPORT_ERROR(f_error_reporter, "AllocateTensors() failed");
    return;
  }

  // Get information about the memory area to use for the model's input.
  f_input = f_interpreter->input(0);
  TF_LITE_REPORT_ERROR(f_error_reporter, "input size : %d bytes\n", f_input->bytes);
  TF_LITE_REPORT_ERROR(f_error_reporter, "TF initialize finished.");
  return;
}

// The name of this function is important for Arduino compatibility.
void Run(int type, float* output0, float* output1) {
  // Get image from provider.
  if (kTfLiteOk != GetFaceImage(f_error_reporter, kNumCols, kNumRows, kNumChannels,
                            f_input->data.f, type)) {
    TF_LITE_REPORT_ERROR(f_error_reporter, "Image capture failed.");
  }

  // Run the model on this input and make sure it succeeds.
  if (kTfLiteOk != f_interpreter->Invoke()) {
    TF_LITE_REPORT_ERROR(f_error_reporter, "Invoke failed.");
  }

  TF_LITE_REPORT_ERROR(f_error_reporter, "Finish run nn.");
  TfLiteTensor* o0 = f_interpreter->output(0);
  TfLiteTensor* o1 = f_interpreter->output(1);

  memcpy(output0, o0->data.f, ko0Size);
  memcpy(output1, o1->data.f, ko1Size);
  return;
}
