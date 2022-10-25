############################################################################
# externals/awsiot/awsiot.mk
#
#   Copyright 2019 Sony Corporation
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

AWSCOREDIR = $(EXTERNAL_DIR)$(DELIM)awsiot$(DELIM)aws-iot-device-sdk-embedded-C
AWSPLATDIR = $(AWSCOREDIR)$(DELIM)platform$(DELIM)linux

ROOTDEPPATH += --dep-path $(AWSCOREDIR)$(DELIM)src
ROOTDEPPATH += --dep-path $(AWSCOREDIR)$(DELIM)external_libs$(DELIM)jsmn
ROOTDEPPATH += --dep-path $(AWSPLATDIR)$(DELIM)common
ROOTDEPPATH += --dep-path $(AWSPLATDIR)$(DELIM)mbedtls
ROOTDEPPATH += --dep-path $(AWSPLATDIR)$(DELIM)pthread

VPATH += :$(AWSCOREDIR)$(DELIM)src
VPATH += :$(AWSCOREDIR)$(DELIM)external_libs$(DELIM)jsmn
VPATH += :$(AWSPLATDIR)$(DELIM)common
VPATH += :$(AWSPLATDIR)$(DELIM)mbedtls
VPATH += :$(AWSPLATDIR)$(DELIM)pthread

ifeq ($(WINTOOL),y)
CFLAGS += -I"${shell cygpath -w $(AWSCOREDIR)$(DELIM)include}"
CFLAGS += -I"${shell cygpath -w $(AWSCOREDIR)$(DELIM)external_libs$(DELIM)jsmn}"
CFLAGS += -I"${shell cygpath -w $(AWSCOREDIR)$(DELIM)platform$(DELIM)linux$(DELIM)common}"
CFLAGS += -I"${shell cygpath -w $(AWSCOREDIR)$(DELIM)platform$(DELIM)linux$(DELIM)mbedtls}"
CFLAGS += -I"${shell cygpath -w $(AWSCOREDIR)$(DELIM)platform$(DELIM)linux$(DELIM)pthread}"
else
CFLAGS += -I$(AWSCOREDIR)$(DELIM)include
CFLAGS += -I$(AWSCOREDIR)$(DELIM)external_libs$(DELIM)jsmn
CFLAGS += -I$(AWSCOREDIR)$(DELIM)platform$(DELIM)linux$(DELIM)common
CFLAGS += -I$(AWSCOREDIR)$(DELIM)platform$(DELIM)linux$(DELIM)mbedtls
CFLAGS += -I$(AWSCOREDIR)$(DELIM)platform$(DELIM)linux$(DELIM)pthread
endif

CSRCS += aws_iot_jobs_interface.c
CSRCS += aws_iot_jobs_json.c
CSRCS += aws_iot_jobs_topics.c
CSRCS += aws_iot_jobs_types.c
CSRCS += aws_iot_json_utils.c
CSRCS += aws_iot_mqtt_client.c
CSRCS += aws_iot_mqtt_client_common_internal.c
CSRCS += aws_iot_mqtt_client_connect.c
CSRCS += aws_iot_mqtt_client_publish.c
CSRCS += aws_iot_mqtt_client_subscribe.c
CSRCS += aws_iot_mqtt_client_unsubscribe.c
CSRCS += aws_iot_mqtt_client_yield.c
CSRCS += aws_iot_shadow_actions.c
CSRCS += aws_iot_shadow.c
CSRCS += aws_iot_shadow_json.c
CSRCS += aws_iot_shadow_records.c
CSRCS += timer.c
CSRCS += network_mbedtls_wrapper.c
CSRCS += threads_pthread_wrapper.c
CSRCS += jsmn.c

