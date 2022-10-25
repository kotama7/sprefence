/****************************************************************************
 * apps/lte/alt1250/alt1250_util.h
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

#ifndef __LTE_ALT1250_ALT1250_UTIL_H__
#define __LTE_ALT1250_ALT1250_UTIL_H__

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "alt1250_daemon.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef MIN
#  define MIN(a,b)  (((a) < (b)) ? (a) : (b))
#endif

#ifndef ARRAY_SZ
#  define ARRAY_SZ(array) (sizeof(array)/sizeof(array[0]))
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

void alt1250_saveapn(FAR struct alt1250_s *dev, FAR lte_apn_setting_t *apn);
void alt1250_getapn(FAR struct alt1250_s *dev, FAR lte_apn_setting_t *apn);

#endif  /* __LTE_ALT1250_ALT1250_UTIL_H__ */
