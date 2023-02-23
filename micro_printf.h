// Copyright (c) 2023 Cesanta Software Limited
// All rights reserved
//
// This software is dual-licensed: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License version 3.0 as
// published by the Free Software Foundation. For the terms of this
// license, see http://www.gnu.org/licenses/
//
// You are free to use this software under the terms of the GNU General
// Public License, but WITHOUT ANY WARRANTY; without even the implied
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// Alternatively, you can license this software under a commercial
// license, as set out in https://mongoose.ws/licensing/
//
// SPDX-License-Identifier: AGPL-3.0 or commercial

#pragma once

#include <stdarg.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Low level basic functions
size_t m_vxprintf(void (*)(char, void *), void *, const char *fmt, va_list *);
size_t m_xprintf(void (*)(char, void *), void *, const char *fmt, ...);

// Convenience wrappers around m_xprintf
size_t m_vsnprintf(char *buf, size_t len, const char *fmt, va_list *ap);
size_t m_snprintf(char *, size_t, const char *fmt, ...);

// Pre-defined %M formatting functions
size_t m_fmt_ip4(void (*out)(char, void *), void *arg, va_list *ap);
size_t m_fmt_ip6(void (*out)(char, void *), void *arg, va_list *ap);
size_t m_fmt_mac(void (*out)(char, void *), void *arg, va_list *ap);
size_t m_fmt_b64(void (*out)(char, void *), void *arg, va_list *ap);
size_t m_fmt_esc(void (*out)(char, void *), void *arg, va_list *ap);
size_t m_fmt_quo(void (*out)(char, void *), void *arg, va_list *ap);

#ifdef __cplusplus
}
#endif
