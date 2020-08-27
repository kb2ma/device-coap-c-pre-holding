/* EdgeX device service to receive an int value asynchronously via CoAP server */

/*
 * Copyright (c) 2020
 * Ken Bannister
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <unistd.h>
#include <stdarg.h>

#include "devsdk/devsdk.h"
#include "device-coap.h"

#define ERR_CHECK(x) if (x.code) { fprintf (stderr, "Error: %d: %s\n", x.code, x.reason); devsdk_service_free (service); free (impl); return x.code; }

#define ERR_BUFSZ 1024


static bool coap_init
(
  void *impl,
  struct iot_logger_t *lc,
  const iot_data_t *config
)
{
  (void) config;
  coap_driver *driver = (coap_driver *) impl;
  driver->lc = lc;
  iot_log_debug(driver->lc,"Init");
  return true;
}

static void coap_reconfigure (void *impl, const iot_data_t *config) {}

static bool coap_get_handler
(
  void *impl,
  const char *devname,
  const devsdk_protocols *protocols,
  uint32_t nreadings,
  const devsdk_commandrequest *requests,
  devsdk_commandresult *readings,
  const devsdk_nvpairs *qparms,
  iot_data_t **exception
)
{
  (void) impl;
  (void) devname;
  (void) protocols;
  (void) nreadings;
  (void) requests;
  (void) readings;
  (void) qparms;
  (void) exception;
  return true;
}

static bool coap_put_handler
(
  void *impl,
  const char *devname,
  const devsdk_protocols *protocols,
  uint32_t nvalues,
  const devsdk_commandrequest *requests,
  const iot_data_t *values[],
  iot_data_t **exception
)
{
  (void) impl;
  (void) devname;
  (void) protocols;
  (void) nvalues;
  (void) requests;
  (void) values;
  (void) exception;
  return true;
}

static void coap_stop (void *impl, bool force) {}

int main (int argc, char *argv[])
{
  coap_driver * impl = malloc (sizeof (coap_driver));
  memset (impl, 0, sizeof (coap_driver));

  devsdk_error e;
  e.code = 0;

  /* Device Callbacks */
  devsdk_callbacks coapImpls =
  {
    coap_init,
    coap_reconfigure,
    NULL,              /* discovery */
    coap_get_handler,
    coap_put_handler,
    coap_stop,
    NULL,              /* device added */
    NULL,              /* device updated */
    NULL,              /* device removed */
    NULL,              /* auto-event started */
    NULL               /* auto-event stopped */
  };

  /* Initialize a new device service */
  devsdk_service_t *service = devsdk_service_new
    ("device-coap", VERSION, impl, coapImpls, &argc, argv, &e);
  ERR_CHECK (e);
  impl->service = service;

  int n = 1;
  uint8_t psk_key[16];
  int keylen = 0;
  while (n < argc)
  {
    if (strcmp (argv[n], "-h") == 0 || strcmp (argv[n], "--help") == 0)
    {
      printf ("Usage: device-coap [options]\n");
      printf ("\n");
      printf ("Options:\n");
      printf ("  -k <key>    \t\tUse DTLS PSK security, key max 16 chars\n");
      printf ("  -h, --help\t\tShow this text\n");
      return 0;
    }
    else if (strcmp (argv[n], "-k") == 0)
    {
      if (argc > n + 1)
      {
        keylen = strlen(argv[n + 1]);
        if (keylen > 16)
        {
          printf ("Key too long\n");
          return 1;
        }
        memcpy(psk_key, argv[n+1], keylen);
        n += 2;
      }
      else
      {
        printf ("Missing value of key\n");
        return 1;
      }
    }
    else
    {
      printf ("%s: Unrecognized option %s\n", argv[0], argv[n]);
      return 1;
    }
  }

  /* Start the device service*/
  devsdk_service_start (service, NULL, &e);
  ERR_CHECK (e);

  /* Run CoAP server */
  run_server(impl, psk_key, keylen);

  /* Stop the device service */
  devsdk_service_stop (service, true, &e);
  ERR_CHECK (e);

  devsdk_service_free (service);
  free (impl);
  return 0;
}
