############################################################################
# externals/awsiot/awsiot_config.mk
#
#   Copyright 2021 Sony Semiconductor Solutions Corporation
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in
#    the documentation and/or other materials provided with the
#    distribution.
# 3. Neither the name of Sony Semiconductor Solutions Corporation nor
#    the names of its contributors may be used to endorse or promote
#    products derived from this software without specific prior written
#    permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
# OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
# AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
# ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
############################################################################

# Log option
ifeq ($(CONFIG_EXTERNALS_AWSIOT_ENABLE_IOT_DEBUG),y)
  CFLAGS += -DENABLE_IOT_DEBUG
endif

ifeq ($(CONFIG_EXTERNALS_AWSIOT_ENABLE_IOT_TRACE),y)
  CFLAGS += -DENABLE_IOT_TRACE
endif

ifeq ($(CONFIG_EXTERNALS_AWSIOT_ENABLE_IOT_INFO),y)
  CFLAGS += -DENABLE_IOT_INFO
endif

ifeq ($(CONFIG_EXTERNALS_AWSIOT_ENABLE_IOT_WARN),y)
  CFLAGS += -DENABLE_IOT_WARN
endif

ifeq ($(CONFIG_EXTERNALS_AWSIOT_ENABLE_IOT_ERROR),y)
  CFLAGS += -DENABLE_IOT_ERROR
endif
