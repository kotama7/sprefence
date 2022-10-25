/****************************************************************************
 * modules/asmp/rawelf/rawelf_unload.c
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

#include <stdlib.h>
#include <debug.h>

#include <nuttx/kmalloc.h>
#include <mm/tile.h>

#include "rawelf.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

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
 * Name: rawelf_unload
 *
 * Description:
 *   This function unloads the object from memory. This essentially undoes
 *   the actions of rawelf_load.  It is called only under certain error
 *   conditions after the module has been loaded but not yet started.
 *
 * Returned Value:
 *   0 (OK) is returned on success and a negated errno is returned on
 *   failure.
 *
 ****************************************************************************/

int rawelf_unload(struct rawelf_loadinfo_s *loadinfo)
{
  /* Free all working buffers */

  rawelf_freebuffers(loadinfo);

  /* If there is an allocation for the ELF image, free it */

  if (loadinfo->textalloc != 0)
    {
      tile_free((FAR void *)loadinfo->textalloc, loadinfo->textsize + loadinfo->datasize);
    }

  /* Clear out all indications of the allocated address environment */

  loadinfo->textalloc = 0;
  loadinfo->dataalloc = 0;
  loadinfo->textsize  = 0;
  loadinfo->datasize  = 0;

  /*
   * XXX: Disable this logic because static constructors and destructors
   * are not supported yet in ASMP loader.
   */
#if 0
  /* Release memory used to hold static constructors and destructors */

#ifdef CONFIG_BINFMT_CONSTRUCTORS
  if (loadinfo->ctoralloc != 0)
    {
      kumm_free(loadinfo->ctoralloc);
      loadinfo->ctoralloc = NULL;
    }

   loadinfo->ctors   = NULL;
   loadinfo->nctors  = 0;

  if (loadinfo->dtoralloc != 0)
    {
      kumm_free(loadinfo->dtoralloc);
      loadinfo->dtoralloc = NULL;
    }

   loadinfo->dtors   = NULL;
   loadinfo->ndtors  = 0;
#endif
#endif

  return OK;
}

