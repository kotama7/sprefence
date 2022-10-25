/****************************************************************************
 * modules/asmp/rawelf/rawelf_init.c
 *
 *   Copyright 2018 Sony Semiconductor Solutions Corporation
 *
 *   Copyright (C) 2012 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <sdk/config.h>

#include <sys/stat.h>

#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <elf32.h>
#include <debug.h>
#include <errno.h>

#include <nuttx/binfmt/elf.h>

#include "rawelf.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* CONFIG_DEBUG, CONFIG_DEBUG_VERBOSE, and CONFIG_DEBUG_BINFMT have to be
 * defined or CONFIG_RAWELF_DUMPBUFFER does nothing.
 */

#if !defined(CONFIG_DEBUG_VERBOSE) || !defined (CONFIG_DEBUG_BINFMT)
#  undef CONFIG_RAWELF_DUMPBUFFER
#endif

#ifdef CONFIG_RAWELF_DUMPBUFFER
# define rawelf_dumpbuffer(m,b,n) bvdbgdumpbuffer(m,b,n)
#else
# define rawelf_dumpbuffer(m,b,n)
#endif

/****************************************************************************
 * Private Constant Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: rawelf_init
 *
 * Description:
 *   This function is called to configure the library to process an ELF
 *   program binary.
 *
 * Returned Value:
 *   0 (OK) is returned on success and a negated errno is returned on
 *   failure.
 *
 ****************************************************************************/

int rawelf_init(FAR struct rawelf_loadinfo_s *loadinfo)
{
  int ret;

  ret = rawelf_read(loadinfo, (FAR uint8_t *)&loadinfo->ehdr, sizeof(Elf32_Ehdr), 0);
  if (ret < 0)
    {
      berr("Failed to read ELF header: %d\n", ret);
      return ret;
    }

  rawelf_dumpbuffer("ELF header", (FAR const uint8_t *)&loadinfo->ehdr, sizeof(Elf32_Ehdr));

  /* Verify the ELF header */

  ret = rawelf_verifyheader(&loadinfo->ehdr);
  if (ret < 0)
    {
      /* This may not be an error because we will be called to attempt loading
       * EVERY binary.  If rawelf_verifyheader() does not recognize the ELF header,
       * it will -ENOEXEC whcih simply informs the system that the file is not an
       * ELF file.  rawelf_verifyheader() will return other errors if the ELF header
       * is not correctly formed.
       */

      berr("Bad ELF header: %d\n", ret);
      return ret;
    }

  return OK;
}

