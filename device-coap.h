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
 * @brief This file defines common artifacts for the CoAP device service.
 */

#include "devsdk/devsdk.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct coap_driver
{
  iot_logger_t * lc;
  devsdk_service_t *service;
} coap_driver;


int run_server(coap_driver *driver);


#ifdef __cplusplus
}
#endif

#endif
