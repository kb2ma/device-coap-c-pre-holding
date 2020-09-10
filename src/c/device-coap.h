/*
 * Copyright (c) 2020
 * Ken Bannister
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef _DEVICE_COAP_H_
#define _DEVICE_COAP_H_ 1

/**
 * @file
 * @brief Defines common artifacts for the CoAP device service.
 */

#include "devsdk/devsdk.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * device-coap-c specific data included with service callbacks
 */
typedef struct coap_driver
{
  iot_logger_t * lc;
  devsdk_service_t *service;
  const char *coap_proto;          /**< CoAP protocol with device; 'coap' or 'coaps';
                                        from config, must retain for internal use */
} coap_driver;

/**
 * Runs a CoAP server, optionally with PSK security, until a SIGINT or SIGTERM event.
 *
 * @param driver   EdgeX driver
 * @param psk_key  PSK key
 * @param keylen   Length of psk_key, or 0 if nosec
 * @return EXIT_SUCCESS on normal completion
 * @return EXIT_FAILURE if unable to run server
 */
int run_server(coap_driver *driver, const uint8_t *psk_key, int keylen);


#ifdef __cplusplus
}
#endif

#endif
