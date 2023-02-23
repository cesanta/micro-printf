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
// license, as set out in https://www.cesanta.com/licensing/
//
// SPDX-License-Identifier: AGPL-3.0 or commercial

#if defined(_MSC_VER) && _MSC_VER < 1700
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;
typedef unsigned char uint8_t;
typedef char int8_t;
typedef unsigned short uint16_t;
typedef short int16_t;
typedef unsigned int uint32_t;
typedef int int32_t;
#else
#include <stdint.h>
#endif

#include "micro_printf.h"

typedef void (*m_out_t)(char, void *);                  // Output function
typedef size_t (*m_fmt_t)(m_out_t, void *, va_list *);  // %M format function

struct m_buf {
  char *buf;
  size_t size, len;
};

static void m_out_buf(char ch, void *param) {
  struct m_buf *mb = (struct m_buf *) param;
  if (mb->len < mb->size) mb->buf[mb->len] = ch;
  mb->len++;
}

size_t m_vsnprintf(char *buf, size_t len, const char *fmt, va_list *ap) {
  struct m_buf mb = {buf, len, 0};
  size_t n = m_vxprintf(m_out_buf, &mb, fmt, ap);
  if (len > 0) buf[n < len ? n : len - 1] = '\0';  // NUL terminate
  return n;
}

size_t m_snprintf(char *buf, size_t len, const char *fmt, ...) {
  va_list ap;
  size_t n;
  va_start(ap, fmt);
  n = m_vsnprintf(buf, len, fmt, &ap);
  va_end(ap);
  return n;
}

size_t m_fmt_ip4(void (*out)(char, void *), void *arg, va_list *ap) {
  uint8_t *p = va_arg(*ap, uint8_t *);
  return m_xprintf(out, arg, "%d.%d.%d.%d", p[0], p[1], p[2], p[3]);
}

#define U16(p) ((((uint16_t) (p)[0]) << 8) | (p)[1])
size_t m_fmt_ip6(void (*out)(char, void *), void *arg, va_list *ap) {
  uint8_t *p = va_arg(*ap, uint8_t *);
  return m_xprintf(out, arg, "[%x:%x:%x:%x:%x:%x:%x:%x]", U16(&p[0]),
                   U16(&p[2]), U16(&p[4]), U16(&p[6]), U16(&p[8]), U16(&p[10]),
                   U16(&p[12]), U16(&p[14]));
}

size_t m_fmt_mac(void (*out)(char, void *), void *arg, va_list *ap) {
  uint8_t *p = va_arg(*ap, uint8_t *);
  return m_xprintf(out, arg, "%02x:%02x:%02x:%02x:%02x:%02x", p[0], p[1], p[2],
                   p[3], p[4], p[5]);
}

static int m_isdigit(int c) { return c >= '0' && c <= '9'; }

static size_t m_strlen(const char *s) {
  size_t n = 0;
  while (s[n] != '\0') n++;
  return n;
}

#if !defined(NOFLOAT)
static int addexp(char *buf, int e, int sign) {
  int n = 0;
  buf[n++] = 'e';
  buf[n++] = (char) sign;
  if (e > 400) return 0;
  if (e < 10) buf[n++] = '0';
  if (e >= 100) buf[n++] = (char) (e / 100 + '0'), e -= 100 * (e / 100);
  if (e >= 10) buf[n++] = (char) (e / 10 + '0'), e -= 10 * (e / 10);
  buf[n++] = (char) (e + '0');
  return n;
}

static int xisinf(double x) {
  union {
    double f;
    uint64_t u;
  } ieee754 = {x};
  return ((unsigned) (ieee754.u >> 32) & 0x7fffffff) == 0x7ff00000 &&
         ((unsigned) ieee754.u == 0);
}

static int xisnan(double x) {
  union {
    double f;
    uint64_t u;
  } ieee754 = {x};
  return ((unsigned) (ieee754.u >> 32) & 0x7fffffff) +
             ((unsigned) ieee754.u != 0) >
         0x7ff00000;
}

static size_t m_dtoa(char *dst, size_t dstlen, double d, int width) {
  char buf[40];
  int i, s = 0, n = 0, e = 0;
  double t, mul, saved;
  if (d == 0.0) return m_snprintf(dst, dstlen, "%s", "0");
  if (xisinf(d)) return m_snprintf(dst, dstlen, "%s", d > 0 ? "inf" : "-inf");
  if (xisnan(d)) return m_snprintf(dst, dstlen, "%s", "nan");
  if (d < 0.0) d = -d, buf[s++] = '-';

  // Round
  saved = d;
  mul = 1.0;
  while (d >= 10.0 && d / mul >= 10.0) mul *= 10.0;
  while (d <= 1.0 && d / mul <= 1.0) mul /= 10.0;
  for (i = 0, t = mul * 5; i < width; i++) t /= 10.0;
  d += t;
  // Calculate exponent, and 'mul' for scientific representation
  mul = 1.0;
  while (d >= 10.0 && d / mul >= 10.0) mul *= 10.0, e++;
  while (d < 1.0 && d / mul < 1.0) mul /= 10.0, e--;
  // printf(" --> %g %d %g %g\n", saved, e, t, mul);

  if (e >= width) {
    n = (int) m_dtoa(buf, sizeof(buf), saved / mul, width);
    // printf(" --> %.*g %d [%.*s]\n", 10, d / t, e, n, buf);
    n += addexp(buf + s + n, e, '+');
    return m_snprintf(dst, dstlen, "%.*s", n, buf);
  } else if (e <= -width) {
    n = (int) m_dtoa(buf, sizeof(buf), saved / mul, width);
    // printf(" --> %.*g %d [%.*s]\n", 10, d / mul, e, n, buf);
    n += addexp(buf + s + n, -e, '-');
    return m_snprintf(dst, dstlen, "%.*s", n, buf);
  } else {
    for (i = 0, t = mul; t >= 1.0 && s + n < (int) sizeof(buf); i++) {
      int ch = (int) (d / t);
      if (n > 0 || ch > 0) buf[s + n++] = (char) (ch + '0');
      d -= ch * t;
      t /= 10.0;
    }
    // printf(" --> [%g] -> %g %g (%d) [%.*s]\n", saved, d, t, n, s + n, buf);
    if (n == 0) buf[s++] = '0';
    while (t >= 1.0 && n + s < (int) sizeof(buf)) buf[n++] = '0', t /= 10.0;
    if (s + n < (int) sizeof(buf)) buf[n + s++] = '.';
    // printf(" 1--> [%g] -> [%.*s]\n", saved, s + n, buf);
    for (i = 0, t = 0.1; s + n < (int) sizeof(buf) && n < width; i++) {
      int ch = (int) (d / t);
      buf[s + n++] = (char) (ch + '0');
      d -= ch * t;
      t /= 10.0;
    }
  }
  while (n > 0 && buf[s + n - 1] == '0') n--;  // Trim trailing zeros
  if (n > 0 && buf[s + n - 1] == '.') n--;     // Trim trailing dot
  n += s;
  if (n >= (int) sizeof(buf)) n = (int) sizeof(buf) - 1;
  buf[n] = '\0';
  return m_snprintf(dst, dstlen, "%s", buf);
}
#endif

static size_t m_lld(char *buf, int64_t val, int is_signed, int is_hex) {
  const char *letters = "0123456789abcdef";
  uint64_t v = (uint64_t) val;
  size_t s = 0, n, i;
  if (is_signed && val < 0) buf[s++] = '-', v = (uint64_t) (-val);
  // This loop prints a number in reverse order. I guess this is because we
  // write numbers from right to left: least significant digit comes last.
  // Maybe because we use Arabic numbers, and Arabs write RTL?
  if (is_hex) {
    for (n = 0; v; v >>= 4) buf[s + n++] = letters[v & 15];
  } else {
    for (n = 0; v; v /= 10) buf[s + n++] = letters[v % 10];
  }
  // Reverse a string
  for (i = 0; i < n / 2; i++) {
    char t = buf[s + i];
    buf[s + i] = buf[s + n - i - 1], buf[s + n - i - 1] = t;
  }
  if (val == 0) buf[n++] = '0';  // Handle special case
  return n + s;
}

static size_t scpy(void (*o)(char, void *), void *ptr, char *buf, size_t len) {
  size_t i = 0;
  while (i < len && buf[i] != '\0') o(buf[i++], ptr);
  return i;
}

static char m_esc(int c, int esc) {
  const char *p, *esc1 = "\b\f\n\r\t\\\"", *esc2 = "bfnrt\\\"";
  for (p = esc ? esc1 : esc2; *p != '\0'; p++) {
    if (*p == c) return esc ? esc2[p - esc1] : esc1[p - esc2];
  }
  return 0;
}

size_t m_fmt_esc(void (*out)(char, void *), void *param, va_list *ap) {
  const char *s = va_arg(*ap, const char *);
  size_t i, n = 0;
  for (i = 0; s != NULL && s[i] != '\0'; i++) {
    char c = m_esc(s[i], 1);
    if (c) {
      out('\\', param), out(c, param), n += 2;
    } else {
      out(s[i], param);
      n++;
    }
  }
  return n;
}

size_t m_fmt_quo(void (*out)(char, void *), void *param, va_list *ap) {
  m_fmt_t f = va_arg(*ap, m_fmt_t);
  size_t n = 2;
  out('"', param);
  n += f(out, param, ap);
  out('"', param);
  return n;
}

size_t m_fmt_b64(void (*out)(char, void *), void *param, va_list *ap) {
  unsigned len = va_arg(*ap, unsigned);
  uint8_t *buf = va_arg(*ap, uint8_t *);
  size_t i, n = 0;
  const char *t =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  for (i = 0; i < len; i += 3) {
    uint8_t c1 = buf[i], c2 = i + 1 < len ? buf[i + 1] : 0,
            c3 = i + 2 < len ? buf[i + 2] : 0;
    char tmp[4] = {t[c1 >> 2], t[(c1 & 3) << 4 | (c2 >> 4)], '=', '='};
    if (i + 1 < len) tmp[2] = t[(c2 & 15) << 2 | (c3 >> 6)];
    if (i + 2 < len) tmp[3] = t[c3 & 63];
    n += scpy(out, param, tmp, sizeof(tmp));
  }
  return n;
}

size_t m_xprintf(void (*out)(char, void *), void *ptr, const char *fmt, ...) {
  size_t len = 0;
  va_list ap;
  va_start(ap, fmt);
  len = m_vxprintf(out, ptr, fmt, &ap);
  va_end(ap);
  return len;
}

size_t m_vxprintf(void (*out)(char, void *), void *param, const char *fmt,
                  va_list *ap) {
  size_t i = 0, n = 0;
  while (fmt[i] != '\0') {
    if (fmt[i] == '%') {
      size_t j, k, x = 0, is_long = 0, w = 0 /* width */, pr = ~0U /* prec */;
      char pad = ' ', minus = 0, c = fmt[++i];
      if (c == '#') x++, c = fmt[++i];
      if (c == '-') minus++, c = fmt[++i];
      if (c == '0') pad = '0', c = fmt[++i];
      while (m_isdigit(c)) w *= 10, w += (size_t) (c - '0'), c = fmt[++i];
      if (c == '.') {
        c = fmt[++i];
        if (c == '*') {
          pr = (size_t) va_arg(*ap, int);
          c = fmt[++i];
        } else {
          pr = 0;
          while (m_isdigit(c)) pr *= 10, pr += (size_t) (c - '0'), c = fmt[++i];
        }
      }
      while (c == 'h') c = fmt[++i];  // Treat h and hh as int
      if (c == 'l') {
        is_long++, c = fmt[++i];
        if (c == 'l') is_long++, c = fmt[++i];
      }
      if (c == 'p') x = 1, is_long = 1;
      if (c == 'd' || c == 'u' || c == 'x' || c == 'X' || c == 'p'
#if !defined(NOFLOAT)
          || c == 'g' || c == 'f'
#endif
      ) {
        int s = (c == 'd'), h = (c == 'x' || c == 'X' || c == 'p');
        char tmp[40];
        size_t xl = x ? 2 : 0;
#if !defined(NOFLOAT)
        if (c == 'g' || c == 'f') {
          double v = va_arg(*ap, double);
          if (pr == ~0U) pr = 6;
          k = m_dtoa(tmp, sizeof(tmp), v, (int) pr);
        } else
#endif
            if (is_long == 2) {
          int64_t v = va_arg(*ap, int64_t);
          k = m_lld(tmp, v, s, h);
        } else if (is_long == 1) {
          long v = va_arg(*ap, long);
          k = m_lld(tmp, s ? (int64_t) v : (int64_t) (unsigned long) v, s, h);
        } else {
          int v = va_arg(*ap, int);
          k = m_lld(tmp, s ? (int64_t) v : (int64_t) (unsigned) v, s, h);
        }
        for (j = 0; j < xl && w > 0; j++) w--;
        for (j = 0; pad == ' ' && !minus && k < w && j + k < w; j++)
          n += scpy(out, param, &pad, 1);
        n += scpy(out, param, (char *) "0x", xl);
        for (j = 0; pad == '0' && k < w && j + k < w; j++)
          n += scpy(out, param, &pad, 1);
        n += scpy(out, param, tmp, k);
        for (j = 0; pad == ' ' && minus && k < w && j + k < w; j++)
          n += scpy(out, param, &pad, 1);
      } else if (c == 'M') {
        m_fmt_t f = va_arg(*ap, m_fmt_t);
        n += f(out, param, ap);
      } else if (c == 'c') {
        int ch = va_arg(*ap, int);
        out((char) ch, param);
        n++;
      } else if (c == 's') {
        char *p = va_arg(*ap, char *);
        size_t (*f)(void (*)(char, void *), void *, char *, size_t) = scpy;
        if (pr == ~0U) pr = p == NULL ? 0 : m_strlen(p);
        for (j = 0; !minus && pr < w && j + pr < w; j++)
          n += f(out, param, &pad, 1);
        n += f(out, param, p, pr);
        for (j = 0; minus && pr < w && j + pr < w; j++)
          n += f(out, param, &pad, 1);
      } else if (c == '%') {
        out('%', param);
        n++;
      } else {
        out('%', param);
        out(c, param);
        n += 2;
      }
      i++;
    } else {
      out(fmt[i], param), n++, i++;
    }
  }
  return n;
}
