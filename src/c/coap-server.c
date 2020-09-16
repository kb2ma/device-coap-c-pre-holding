/* CoAP server for device-coap-c
 *
 * Copyright (C) 2018 Olaf Bergmann <bergmann@tzi.org>
 * Copyright (c) 2020 Ken Bannister
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <float.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>

#include <coap2/coap.h>
#include "devsdk/devsdk.h"
#include "device-coap.h"

/* Maximum length of a string containing numeric values. */
#define INT32_STR_MAXLEN 10
#define FLOAT64_STR_MAXLEN (DBL_MAX_10_EXP + 2)

#define RESOURCE_SEG1 "a1r"
#define MSG_PAYLOAD_INVALID "payload not valid"

static coap_driver *sdk_ctx;

/* controls input loop */
static int quit = 0;

/* signal handler for input loop */
static void
handle_sig (int signum)
{
  (void)signum;
  quit = 1;
}

/* Builds dst struct from host/port */
static int
resolve_address (const char *host, const char *service, coap_address_t *dst)
{
  struct addrinfo *res, *ainfo;
  struct addrinfo hints;
  int error, len=-1;

  memset (&hints, 0, sizeof (hints));
  memset (dst, 0, sizeof (*dst));
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_family = AF_UNSPEC;

  error = getaddrinfo (host, service, &hints, &res);

  if (error != 0)
  {
    iot_log_info (sdk_ctx->lc, "getaddrinfo: %s\n", gai_strerror (error));
    return error;
  }

  for (ainfo = res; ainfo != NULL; ainfo = ainfo->ai_next)
  {
    switch (ainfo->ai_family)
    {
    case AF_INET6:
    case AF_INET:
      len = dst->size = ainfo->ai_addrlen;
      memcpy (&dst->addr.sin6, ainfo->ai_addr, dst->size);
      goto finish;
    default:
      ;
    }
  }

 finish:
  freeaddrinfo (res);
  return len;
}

/* Caller must free returned iot_data_t */
static iot_data_t*
read_data_float64 (uint8_t *data, size_t len)
{
  /* data conversion requires a null terminated string */
  uint8_t data_str[FLOAT64_STR_MAXLEN+1];
  memset (data_str, 0, FLOAT64_STR_MAXLEN+1);
  memcpy (data_str, data, len);

  char *endptr;
  errno = 0;
  double dbl_val = strtod ((char *)data_str, &endptr);

  if (errno || (*endptr != '\0'))
  {
    iot_log_info (sdk_ctx->lc, "invalid float64 of len %u", len);
    return NULL;
  }

  return iot_data_alloc_f64 (dbl_val);
}

/* Caller must free returned iot_data_t */
static iot_data_t*
read_data_int32 (uint8_t *data, size_t len)
{
  /* data conversion requires a null terminated string */
  uint8_t data_str[INT32_STR_MAXLEN+1];
  memset (data_str, 0, INT32_STR_MAXLEN+1);
  memcpy (data_str, data, len);

  char *endptr;
  errno = 0;
  long int_val = strtol ((char *)data_str, &endptr, 10);

  /* validate strtol conversion, and ensure within range */
  if (errno || (*endptr != '\0') || (int_val < INT32_MIN) || (int_val > INT32_MAX))
  {
    iot_log_info (sdk_ctx->lc, "invalid int32 of len %u", len);
    return NULL;
  }

  return iot_data_alloc_i32 ((int32_t) int_val);
}

/* Caller must free returned iot_data_t */
static iot_data_t*
read_data_string (uint8_t *data, size_t len)
{
  /* must copy request data to append null terminator */
  char *str_data = malloc (len + 1);
  memcpy (str_data, data, len);
  str_data[len] = '\0';

  iot_data_t *iot_data = iot_data_alloc_string(str_data, IOT_DATA_COPY);
  free (str_data);

  return iot_data;
}

/*
 * Parse URI path, expect 3 segments: /a1r/{device-name}/{resource-name}
 *
 * @param[in] request For path to parse
 * @param[out] device Found device
 * @param[out] resource Found resource for device
 * @return true if URI format OK, and device and resource found 
 */
static bool
parse_path (coap_pdu_t *request, devsdk_devices **device_ptr, devsdk_device_resources **resource_ptr)
{
  coap_string_t *uri_path = coap_get_uri_path (request);
  iot_log_debug (sdk_ctx->lc, "URI %s", uri_path->s);
  char *path = (char *)uri_path->s;

  devsdk_devices *device = NULL;
  devsdk_device_resources *resource = NULL;
  
  char *seg = strtok (path, "/");
  bool res = false;
  for (int i = 0; i < 3; i++)
  {
    if (!seg)
    {
      iot_log_info (sdk_ctx->lc, "missing URI segment %u", i);
      break;
    }

    switch (i)
    {
    case 0:
      if (strcmp (seg, RESOURCE_SEG1))
      {
        iot_log_info (sdk_ctx->lc, "invalid URI; segment %u", i);
        goto end_for;
      }
      break;
    case 1:
      if (!(device = devsdk_get_device (sdk_ctx->service, seg)))
      {
        iot_log_info (sdk_ctx->lc, "device not found: %s", seg);
        goto end_for;
      }
      resource = device->resources;
      break;
    case 2:
      for (; resource; resource = resource->next)
      {
        if (!strcmp (resource->request->resname, seg))
        {
          break;
        }
      }
      if (!resource)
      {
        iot_log_info (sdk_ctx->lc, "resource not found: %s", seg);
        goto end_for;
      }
      res = true;
      break;
    }
    seg = strtok (NULL, "/");
  }

 end_for:
  if (res && seg)
  {
    iot_log_info (sdk_ctx->lc, "extra URI segment");
    res = false;
  }
  coap_delete_string (uri_path);

  if (res)
  {
    *device_ptr = device;
    *resource_ptr = resource;
  }
  else if (device)
  {
    devsdk_free_devices (device);
  }
  return res;
}

/*
 * Read data from device initiated CoAP POST to /a1r/{device-name}/{resource-name},
 * and post it via devsdk_post_readings().
 */
static void
data_handler (coap_context_t *context, coap_resource_t *coap_resource,
              coap_session_t *session, coap_pdu_t *request, coap_binary_t *token,
              coap_string_t *query, coap_pdu_t *response)
{
  (void)context;
  (void)coap_resource;
  (void)session;
  (void)request;
  (void)token;
  (void)query;

  /* reject default PUT method */
  if (request->code == COAP_REQUEST_PUT)
  {
    response->code = COAP_RESPONSE_CODE (405);
    return;
  }

  /* Validate URI, expect 3 segments: /a1r/{device-name}/{resource-name} */
  devsdk_devices *device = NULL;
  devsdk_device_resources *resource = NULL;
  if (!parse_path (request, &device, &resource))
  {
    response->code = COAP_RESPONSE_CODE (404);
    goto finish;
  }

  iot_data_t *iot_data = NULL;
  size_t len;
  uint8_t *data;
  if (!coap_get_data (request, &len, &data))
  {
    iot_log_info (sdk_ctx->lc, "invalid data of len %u", len);
    /* finalized after else clause */
  }
  else
  {
    iot_data_type_t resource_type = iot_typecode_type (resource->request->type);
    switch (resource_type)
    {
      case IOT_DATA_ARRAY:
        iot_data = iot_data_alloc_array(data, len, IOT_DATA_UINT8, IOT_DATA_REF);
        break;
      case IOT_DATA_FLOAT64:
        iot_data = read_data_float64 (data, len);
        break;
      case IOT_DATA_INT32:
        iot_data = read_data_int32 (data, len);
        break;
      case IOT_DATA_STRING:
        iot_data = read_data_string (data, len);
        break;
      default:
        iot_log_warn (sdk_ctx->lc, "unsupported resource type %d", resource_type);
        response->code = COAP_RESPONSE_CODE (500);
        goto finish;
    }
  }
  if (!iot_data)
  {
    response->code = COAP_RESPONSE_CODE (400);
    coap_add_data (response, strlen (MSG_PAYLOAD_INVALID), (uint8_t *)MSG_PAYLOAD_INVALID);
    goto finish;
  }

  /* generate and post an event with the data */
  devsdk_commandresult results[1];
  results[0].origin = 0;
  results[0].value = iot_data;

  devsdk_post_readings (sdk_ctx->service, device->devname, resource->request->resname, results);
  iot_data_free (results[0].value);

  response->code = COAP_RESPONSE_CODE (204);

 finish:
  if (device)
  {
    devsdk_free_devices (device);
  }
}

int
run_server (coap_driver *driver)
{
  coap_context_t  *ctx = NULL;
  coap_address_t dst;
  coap_resource_t *resource = NULL;
  coap_endpoint_t *endpoint = NULL;
  int result = EXIT_FAILURE;
  sdk_ctx = driver;
  struct sigaction sa;

  coap_startup ();

  /* Use EdgeX log level */
  coap_log_t log_level;
  switch (sdk_ctx->lc->level)
  {
    case IOT_LOG_ERROR:
      log_level = LOG_ERR;
      break;
    case IOT_LOG_WARN:
      log_level = LOG_WARNING;
      break;
    case IOT_LOG_DEBUG:
      log_level = LOG_DEBUG;
      break;
    default:
      log_level = LOG_INFO;
  }
  coap_set_log_level (log_level);
  /* workaround for tinydtls log level mismatch to avoid excessive debug logging */
  coap_tls_version_t *ver = coap_get_tls_library_version ();
  if (ver->type == COAP_TLS_LIBRARY_TINYDTLS && log_level == LOG_INFO)
  {
    coap_dtls_set_log_level (log_level - 1);
  }
  else
  {
    coap_dtls_set_log_level (log_level);
  }

  /* Resolve destination address where server should be sent. Use CoAP default ports. */
  coap_proto_t proto = COAP_PROTO_UDP;
  char *port = "5683";
  if (driver->security_mode != SECURITY_MODE_NOSEC)
  {
    proto = COAP_PROTO_DTLS;
    port = "5684";
  }
  if (resolve_address ("0.0.0.0", port, &dst) < 0) {
    iot_log_error (sdk_ctx->lc, "failed to resolve address");
    goto finish;
  }

  /* create CoAP context and a client session */
  ctx = coap_new_context (NULL);

  if (!ctx || !(endpoint = coap_new_endpoint (ctx, &dst, proto)))
  {
    iot_log_error (sdk_ctx->lc, "cannot initialize context");
    goto finish;
  }

  if (driver->security_mode == SECURITY_MODE_PSK)
  {
    /* use iterator just to get address of PSK key data */
    iot_data_array_iter_t array_iter;
    iot_data_array_iter (driver->psk_key, &array_iter);
    iot_data_array_iter_next(&array_iter);

    if (!(coap_context_set_psk (ctx, "", (uint8_t *)iot_data_array_iter_value (&array_iter), iot_data_array_length (driver->psk_key))))
    {
      iot_log_error (sdk_ctx->lc, "cannot initialize PSK");
      goto finish;
    }
  }

  /* Creates handler for PUT, which is not what we want... */
  resource = coap_resource_unknown_init (&data_handler);
  /* ... so add POST handler also. */
  coap_register_handler (resource, COAP_REQUEST_POST, &data_handler);
  coap_add_resource (ctx, resource);

  /* setup signal handling for input loop */
  sigemptyset (&sa.sa_mask);
  sa.sa_handler = handle_sig;
  sa.sa_flags = 0;
  sigaction (SIGINT, &sa, NULL);
  sigaction (SIGTERM, &sa, NULL);

  iot_log_info (sdk_ctx->lc, "CoAP %s server started", driver->psk_key ? "PSK" : "nosec");

  while (!quit)
  {
    coap_io_process (ctx, 0);
  }

  result = EXIT_SUCCESS;

 finish:

  coap_free_context (ctx);
  coap_cleanup ();

  return result;
}
