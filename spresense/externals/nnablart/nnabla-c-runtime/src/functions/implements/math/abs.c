// Copyright 2018,2019,2020,2021 Sony Corporation.
// Copyright 2021 Sony Group Corporation.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "../../utilities/accessor.h"
#include "../../utilities/shape.h"
#include <nnablart/config.h>
#include <nnablart/functions.h>

#include <assert.h>

#include <math.h>

#ifdef CONFIG_ABS

typedef struct {
  rt_variable_t *input;
  rt_variable_getter get_input;
  rt_variable_t *output;
  rt_variable_setter set_output;
  int input_size;
  int output_size;
} abs_private_t;

rt_function_error_t exec_abs_generic(rt_function_t *f);

// Abs
rt_function_error_t allocate_abs_local_context(rt_function_t *f) {
  if (f->num_of_inputs != 1) {
    return RT_FUNCTION_ERROR_INVALID_NUM_OF_INPUTS;
  }
  if (f->num_of_outputs != 1) {
    return RT_FUNCTION_ERROR_INVALID_NUM_OF_OUTPUTS;
  }

  abs_private_t *p = rt_malloc_func(sizeof(abs_private_t));
  if (p == 0) {
    return RT_FUNCTION_ERROR_MALLOC;
  }

  f->local_context = (void *)p;
  p->input = f->inputs[0];
  p->get_input = select_getter(p->input);
  p->input_size = calc_shape_size(f->inputs[0]->shape);

  p->output = f->outputs[0];
  p->set_output = select_setter(p->output);
  p->output_size = calc_shape_size(f->outputs[0]->shape);

  if (p->input_size != p->output_size) {
    return RT_FUNCTION_ERROR_INVALID_SHAPE;
  }
  if (p->input->type == NN_DATA_TYPE_FLOAT &&
      p->output->type == NN_DATA_TYPE_FLOAT) {
#ifdef CONFIG_ABS_FLOAT32
    f->exec_func = exec_abs;
#endif /* CONFIG_ABS_FLOAT32 */
  } else {
#ifdef CONFIG_ABS_GENERIC
    f->exec_func = exec_abs_generic;
#endif /* CONFIG_ABS_GENERIC */
  }
  return RT_FUNCTION_ERROR_NOERROR;
}

rt_function_error_t free_abs_local_context(rt_function_t *f) {
  return RT_FUNCTION_ERROR_NOERROR;
}

#ifdef CONFIG_ABS_FLOAT32
rt_function_error_t exec_abs(rt_function_t *f) {
  abs_private_t *p = (abs_private_t *)(f->local_context);
  float *x = (float *)(p->input->data);
  float *y = (float *)(p->output->data);

  int i; // Iterator
  for (i = 0; i < p->output_size; i++) {
    y[i] = fabsf(x[i]);
  }
  return RT_FUNCTION_ERROR_NOERROR;
}
#endif /* CONFIG_ABS_FLOAT32 */

#ifdef CONFIG_ABS_GENERIC
rt_function_error_t exec_abs_generic(rt_function_t *f) {
  abs_private_t *p = (abs_private_t *)(f->local_context);

  int i; // Iterator
  for (i = 0; i < p->output_size; i++) {
    float x = p->get_input(p->input, i);
    p->set_output(p->output, i, fabsf(x));
  }
  return RT_FUNCTION_ERROR_NOERROR;
}
#endif /* CONFIG_ABS_GENERIC */

#endif /* CONFIG_ABS */
