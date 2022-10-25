# Application makefile

OPTIMIZATION_LEVEL := -O3

CC_WARNINGS := \
  -Werror \
  -Wshadow \
  -Wmissing-field-initializers \
  -Wunused-function \
  -Wswitch \
  -Wall \
  -Wextra \
  -Wstrict-aliasing \
  -Wno-unused-parameter \
  -Wno-sign-compare \

PLATFORM_FLAGS := \
  $(SPRESENSE_PLATFORM_FLAGS) \
  -fmessage-length=0 \
  -fno-exceptions \
  -fno-unwind-tables \
  -ffunction-sections \
  -fdata-sections \
  -funsigned-char \
  -fno-delete-null-pointer-checks \
  -fomit-frame-pointer \
  -MMD \
  -mcpu=cortex-m4 \
  -mabi=aapcs \
  -mthumb \
  -mfpu=fpv4-sp-d16 \
  -mfloat-abi=hard

COMMON_FLAGS := \
  -DTF_LITE_STATIC_MEMORY \
  -DTF_LITE_MCU_DEBUG_LOG \
  -DCMSIS_NN \
  $(PLATFORM_FLAGS) \
  $(OPTIMIZATION_LEVEL) \
  $(CC_WARNINGS) \

# Command name (Public function 'int <APPNAME>_main(void)' required)
APPNAME = 

# Application execute priority (Range: 0 ~ 255, Default: 100)
PRIORITY =

# Application stack memory size (Default: 2048)
STACKSIZE = 32768

# Main source code
MAINSRC = face_detect_main.cxx

# Additional C source files (*.c)
CSRCS =

# Additional C++ source files (*.cxx)
CXXSRCS = gpio_control.cxx hazard_assesment.cxx main_handler.cxx non_max_suppression.cxx ssd_anchors.cxx tensors_to_detections.cxx

# Additional assembler source files (*.S)
ASRCS =

# C compiler flags
CCFLAGS := \
	-std=c11 \
  $(COMMON_FLAGS)

# C++ compiler flags
CXXFLAGS := \
  -std=c++11 \
  -fno-rtti \
  -fno-use-cxa-atexit \
  -fno-threadsafe-statics \
  $(COMMON_FLAGS)

include $(SPRESENSE_HOME)/.vscode/application.mk
