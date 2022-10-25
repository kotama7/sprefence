/****************************************************************************
 * examples/lte_azureiot/azureiot_if.c
 *
 *   Copyright 2020 Sony Semiconductor Solutions Corporation
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
 * 3. Neither the name of Sony Semiconductor Solutions Corporation nor
 *    the names of its contributors may be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
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

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <alloca.h>
#include "generate_sas.h"
#include "azureiot_if.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define AZURE_IOT_HUB_HOST_NAME       "azure-devices.net"
#define AZURE_SAS_EXPIRATION_SEC      600
#define AZURE_JPARSE_NUM              (6)

#if 1
#define LOGF(...)
#else
#define LOGF(...)          printf(__VA_ARGS__)
#endif

/****************************************************************************
 * Private types
 ****************************************************************************/

/****************************************************************************
 * Private data
 ****************************************************************************/

static char _eTags[64];
static char _correlationId[256];
static char _hostName[64];
static char _containerName[64];
static char _blobName[64];
static char _sasToken[256];
static char _sas[256];
static char _isSuccess[8];
static char _statusCode[8];

/* SAS URI information */

static struct
{
  const char *name;
  char       *buf;
  int         size;
}
jparse[AZURE_JPARSE_NUM] =
{
  {"\"correlationId\"", _correlationId, sizeof(_correlationId)},
  {"\"hostName\"",      _hostName,      sizeof(_hostName)     },
  {"\"containerName\"", _containerName, sizeof(_containerName)},
  {"\"blobName\"",      _blobName,      sizeof(_blobName)     },
  {"\"sasToken\"",      _sasToken,      sizeof(_sasToken)     },
  {NULL,                NULL,           0                     },
};

/****************************************************************************
 * Private functions
 ****************************************************************************/

static char *strnstr(const char *str,
                     int         buflen,
                     const char *substr,
                     int         subbuflen)
{
  /* Support "strstr" function overrun */

  char  *cp  = NULL;
  int    len = strnlen(substr, subbuflen);

  if (len > buflen)
    {
      return NULL;
    }

  for (int i = 0; i < buflen && *str; i++, str++)
    {
      if (strncmp(str, substr, len) == 0)
        {
          cp = (char *)str;
          break;
        }
    }

  return cp;
}

/* ------------------------------------------------------------------------ */
static int http_get_etags(const char *buffer, int buffer_size)
{
  /* Get Etags from Azure response */

  int          res = ERROR;
  char        *str;
  const char  *target = "ETag: ";

  memset(_eTags, 0, sizeof(_eTags));

  str = strnstr((char *)buffer, buffer_size, target, strlen(target));

  if (str)
    {
      memcpy(_eTags, str + 7, 36);
      res = OK;
    }

  return res;
}

/* ------------------------------------------------------------------------ */
static int jparse_lite(const char *buffer, int buffer_size)
{
  int i = 0;
  int res = 0;

  /* Get SAS URI from Azure response */

  for (i = 0; i < AZURE_JPARSE_NUM && jparse[i].name; i++)
    {
      char  *str = strnstr((char *)buffer, buffer_size, jparse[i].name, 
                            strlen(jparse[i].name));
      char  *cp;

      memset(jparse[i].buf, 0, jparse[i].size);

      if (str == NULL)
        {
          res = -1;
          continue;
        }

      str += strlen(jparse[i].name) + 2;

      for (cp = jparse[i].buf;
           cp - jparse[i].buf < jparse[i].size &&
           str - buffer < buffer_size &&
           *str && *str != '\"'; str++, cp++)
        {
          *cp = *str;
        }

      if (jparse[i].buf[0] == '\0')
        {
          res = -1;
        }
      else
        {
          LOGF("%s: %s\n", jparse[i].name, jparse[i].buf);
        }
    }

  return res;
}

/* ------------------------------------------------------------------------ */
static char *sas_token(struct azureiot_info *info)
{
  /* Create Shared Access Signatures */

  char       *url;
  int         len;
  const char *target = "%s.%s/devices/%s";

  len = strlen(target)
      + strnlen(info->IoTHubName, AZURE_IOT_INFO_NAME_MAX_SIZE)
      + strlen(AZURE_IOT_HUB_HOST_NAME)
      + strnlen(info->DeviceID, AZURE_IOT_INFO_PRIKEY_MAX_SIZE);

  url = alloca(len);

  snprintf(url,
          len,
          target,
          info->IoTHubName,
          AZURE_IOT_HUB_HOST_NAME,
          info->DeviceID);

  if (generate_sas_token(_sas,
                         sizeof(_sas),
                         url,
                         info->PrimaryKey,
                         NULL,
                         AZURE_SAS_EXPIRATION_SEC) < 0)
    {
      return NULL;
    }

  return _sas;
}

/* ------------------------------------------------------------------------ */
static int create_command(struct azureiot_info *info,
                          char                 *request,
                          int                   request_size,
                          const char           *cmd,
                          const char           *function,
                          const char           *content_type,
                          const char           *content)
{
  int         len;
  const char *target =
    "%s /devices/%s/%s?api-version=2018-06-30 HTTP/1.1\r\n"
    "HOST: %s.%s\r\n"
    "Authorization: %s\r\n"
    "Connection: close\r\n";
  const char *target_content_type = "Content-Type: %s\r\n";
  const char *target_content_len  = "Content-Length: %d\r\n";
  const char *sas;

  /* Calculate required string size */

  len = strlen(target)
      + strlen(cmd)
      + strlen(function)
      + strlen("\r\n");

  if (content_type[0] != '\0')
    {
      /* When setting Content-type */

      len += strlen(target_content_type);
      len += strlen(content_type);
    }

  if (content[0] != '\0')
    {
      /* When there is Body data */

      len += strlen(target_content_len);
      len += strlen(content);
    }

  /* Check buffer size */

  if (len >= request_size)
    {
      return ERROR;
    }

  sas = sas_token(info);

  if (sas == NULL)
    {
      return ERROR;
    }

  /* Output HTTP command */

  len = snprintf(request,
                request_size,
                target,
                cmd,
                info->DeviceID,
                function,
                info->IoTHubName,
                AZURE_IOT_HUB_HOST_NAME,
                sas);

  if (content_type[0] != '\0')
    {
      len += snprintf(request + len,
                     request_size - len,
                     target_content_type,
                     content_type); 
    }

  if (content[0] != '\0')
    {
      len += snprintf(request + len,
                     request_size - len,
                     target_content_len,
                     strlen(content)); 
    }

  /* Output borders and content */

  len += snprintf(request + len,
                  request_size - len, "\r\n%s", content); 

  LOGF("\n\n***%s***\n\n", request);

  return len;
}

/****************************************************************************
 * Public functions
 ****************************************************************************/

int azureiot_get_hostname(struct azureiot_info *info,
                          char                 *hostname,
                          int                   hostname_size)
{
  /* Output host name */

  const char  *target = "%s.%s";

  int   len = strlen(info->IoTHubName)
            + strlen(AZURE_IOT_HUB_HOST_NAME);

  if (len >= hostname_size)
    {
      return ERROR;
    }

  return snprintf(hostname,
                  hostname_size,
                  target,
                  info->IoTHubName,
                  AZURE_IOT_HUB_HOST_NAME);
}

/* ------------------------------------------------------------------------ */
int azureiot_create_sendmsg(struct azureiot_info *info,
                            char                 *request,
                            int                   request_size,
                            const char           *post_message)
{
  return create_command(info,
                        request,
                        request_size,
                        "POST",
                        "messages/events",
                        "",
                        post_message);
}

/* ------------------------------------------------------------------------ */
int azureiot_create_recvmsg(struct azureiot_info *info,
                            char                 *request,
                            int                   request_size)

{
  return create_command(info,
                        request,
                        request_size,
                        "GET",
                        "messages/deviceBound",
                        "",
                        "");
}

/* ------------------------------------------------------------------------ */
int azureiot_create_deletemsg(struct azureiot_info *info,
                              char                 *io_buffer,
                              int                   io_buffer_size)
{
  /* Get Etag from Azure response */

  if (http_get_etags(io_buffer, io_buffer_size) < 0)
    {
      return ERROR;
    }

  const char *target = "messages/deviceBound/%s";
  char       *funcs;
  size_t     size = strlen(target) + strnlen(_eTags, sizeof(_eTags)) + 1;

  funcs = alloca(size);

  snprintf(funcs, size, target, _eTags);

  return create_command(info,
                        io_buffer,
                        io_buffer_size,
                        "DELETE",
                        funcs,
                        "",
                        "");
}

/* ------------------------------------------------------------------------ */
int azureiot_create_fileinfo_msg(struct azureiot_info *info,
                                 char                 *request,
                                 int                   request_size,
                                 const char           *file_name)
{
  const char *target = "{\"blobName\":\"%s\"}";
  char       *body;
  size_t     size = strlen(target) + strlen(file_name) + 1;
  

  /* Create body string */

  body = alloca(size);

  snprintf(body, size, "{\"blobName\":\"%s\"}", file_name);

  return create_command(info,
                        request,
                        request_size,
                        "POST",
                        "files",
                        "application/json",
                        body);
}

/* ------------------------------------------------------------------------ */
int azureiot_create_notifymsg(struct azureiot_info *info,
                                       char                 *request,
                                       int                   request_size,
                                       bool                  is_success,
                                       int                   status_code)
{
  const char *target = AZURE_IOT_NOTIFY_STATUS_BODY_FORMAT;
  char       *body;
  size_t     size;

  if (is_success)
    {
      snprintf(_isSuccess, sizeof(_isSuccess), "%s", "true");
    }
  else
    {
      snprintf(_isSuccess, sizeof(_isSuccess), "%s", "false");
    }

  snprintf(_statusCode, sizeof(_statusCode), "%d", status_code);

  size = strlen(target) + strlen(_correlationId)
                        + strlen(_isSuccess)
                        + strlen(_statusCode) + 1;

  /* Create body string */

  body = alloca(size);

  snprintf(body, size, AZURE_IOT_NOTIFY_STATUS_BODY_FORMAT,
                       _correlationId,
                       _isSuccess,
                       _statusCode);

  return create_command(info,
                        request,
                        request_size,
                        "POST",
                        "files/notifications",
                        "application/json",
                        body);
}

/* ------------------------------------------------------------------------ */
int azureiot_create_uploadmsg(char *io_buffer,
                              int   io_buffer_size,
                              char *hostname,
                              int   hostname_size,
                              int   upload_file_size)
{
  const char  http_request[] =
    "%s /%s/%s%s HTTP/1.1\r\n"
    "HOST: %s\r\n"
    "Content-Type: application/octet-stream\r\n"
    "x-ms-blob-type: BlockBlob\r\n";
  const char  target_content_len[]  =
    "Content-Length: %d\r\n";
  int         len;
  const char *method;

 /* Get SAS URI */

  if (jparse_lite(io_buffer, io_buffer_size) < 0)
    {
      return ERROR;
    }

  /* Calculate required string size */

  len = strlen(http_request)
      + strlen(target_content_len)
      + strlen(_containerName)
      + strlen(_blobName)
      + strlen(_sasToken)
      + strlen(_hostName)
      + strlen("\r\n");

  if (upload_file_size > 0)
    {
      /* Upload  */

      len += strlen(target_content_len);

      /* Digit calculation */

      for (int i = upload_file_size; 0 < i; i /= 10)
        {
          len++;
        }

      method = "PUT";
    }
  else
    {
      /* Download  */

      method = "GET";
    }

  len += strlen(method);

  if (len >= io_buffer_size)
    {
      return ERROR;
    }

  /* Output AzureStorage hostname */

  strncpy(hostname, _hostName, hostname_size);

  /* Output HTTP command */

  len = snprintf(io_buffer,
                io_buffer_size,
                http_request,
                method,
                _containerName,
                _blobName,
                _sasToken,
                _hostName);

  if (upload_file_size > 0)
    {
      /* Set upload size */

      len += snprintf(io_buffer + len,
                    io_buffer_size - len,
                    target_content_len,
                    upload_file_size);
    }

  /* Set boundaries */

  len += snprintf(io_buffer + len, io_buffer_size - len, "\r\n");

  return len;
}

/* ------------------------------------------------------------------------ */
int azureiot_create_downloadmsg(char *io_buffer,
                                int   io_buffer_size,
                                char  *hostname,
                                int    hostname_size)
{
  return azureiot_create_uploadmsg(io_buffer,
                                   io_buffer_size,
                                   hostname,
                                   hostname_size,
                                   0);
}
