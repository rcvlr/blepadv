/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#ifndef H_BLEPADV_
#define H_BLEPADV_

#include "os/mynewt.h"
#include "modlog/modlog.h"
#ifdef __cplusplus
extern "C" {
#endif

/** Misc. */
void print_bytes(const uint8_t *bytes, int len);
void print_mbuf(const struct os_mbuf *om);
char *addr_str(const void *addr);

#ifdef __cplusplus
}
#endif

#endif
