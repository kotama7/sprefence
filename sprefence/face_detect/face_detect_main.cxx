#include <nuttx/config.h>
#include <cstdio>
#include <math.h>

#include "tensorflow/lite/micro/spresense/debug_log_callback.h"

#include "main_handler.h"
#include "gpio_control.h"
#include "hazard_assesment.h"

#ifndef CONFIG_DEBUG_FEATURES
#undef CONFIG_DEBUG_CXX
#endif

#ifdef CONFIG_DEBUG_CXX
#define cxxinfo _info
#else
#define cxxinfo(x...)
#endif

static void debug_log_printf(const char *s)
{
  printf(s);
}

extern "C"
{
  int main(int argc, char *argv[])
  {
    RegisterDebugLogCallback(debug_log_printf);

    Setup();

    int nextType = 0;
    int state = -1; // -1 is found, 0 is not found at 0, 1 is not found at 1, 2 is not found at 2, 3 is not found other. 
    float x = -10.0f;
    float y = -10.0f;
    float boxSize = -10.0f;

    while (true)
    {
      float beforeX = x;
      FaceLoop(nextType, &x, &y, &boxSize);
      if (boxSize < 0){
        if (state < 0 && nextType > 2){
          if (beforeX < 0.33){
            nextType = 1;
          }
          else if (beforeX < 0.67)
          {
            nextType = 0;
          }
          else
          {
            nextType = 2;
          }
        }
        else
        {
          state += 1;
          state %= 3;
          nextType = (state + 1 % 3);
        }
      }
      else
      {
        state = -1;
        if (boxSize < 0.55)
        {
          float idxX = x * 4;
          float idxY = y * 3;
          idxX = idxX < 0 ? 0 : idxX;
          idxX = idxX > 4 ? 4 : idxX;
          idxY = idxY < 0 ? 0 : idxY;
          idxY = idxY > 3 ? 3 : idxY;
          nextType = (int)floor(idxY) * 4 + (int)floor(idxX);
        }
        else
        {
          if (beforeX < 0.33){
            nextType = 1;
          }
          else if (beforeX < 0.67)
          {
            nextType = 0;
          }
          else
          {
            nextType = 2;
          }
        }
        // Load();
        if (IsHazard(x, y)){
          FenceOn();
        }else{
          FenceOff();
        }
      }
    }
    

    return 0;
  }
}
