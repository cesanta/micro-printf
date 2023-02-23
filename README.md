# Tiny extendable printf for microcontrollers

[![License: AGPL/Commercial](https://img.shields.io/badge/License-AGPL%20or%20Commercial-green.svg)](https://opensource.org/licenses/agpl-3.0.php)
[![Build+Test Status](https://github.com/cesanta/micro-printf/workflows/build/badge.svg)](https://github.com/cesanta/micro-printf/actions)
[![Code Coverage](https://codecov.io/gh/cesanta/micro-printf/branch/master/graph/badge.svg)](https://codecov.io/gh/cesanta/micro-printf)

This is a minimal `*printf()` implementation optimised for embedded systems,
which supports all major format specifiers, including a floating point `%f` and
`%g` specifiers.  Also it supports a non-standard `%M` format specifier which
allows to use user-defined format function. Helper functions are provided to
print IP addresses, JSON-escaped strings, base64 data, etc.

## Features

- Source code is both ISO C and ISO C++ compliant
- Works with any compiler, starting from MSVC 98 to modern Clang
- Tiny size: see Footprint section below
- Supports all major format specifiers:
- Supports a non-standard `%M` specifier which allows for custom formats
- Extensively tested

## Usage example

```c
// Print into a buffer
char buf[100];
m_snprintf(buf, sizeof(buf), "Hello, %s! Float value: %g", "world", 1.234);
m_snprintf(buf, sizeof(buf), "Substring: %.*s", 3, "foobar");
m_snprintf(buf, sizeof(buf), "Length, padding: %#02x", 123);
m_snprintf(buf, sizeof(buf), "Alignment: %-15s", "hi");

// Print to the UART
void out(char ch, void *param) {
  HAL_UART_Transmit(param, &ch, 1, 100);
}
...
m_xprintf(out, USART3, "Hello, %s!\n", "world");
```

## API reference

### m\_xprintf(), m\_vxprintf()
```c
size_t m_vxprintf(void (*fn)(char, void *), void *p, const char *fmt, va_list *);
size_t m_xprintf(void (*fn)(char, void *), void *p, const char *fmt, ...);
```

Print formatted string using output function `fn()`. Parameters:
- `fn` - a user-defined output function
- `p` - an arbitrary parameter to the `fn()` function
- `fmt` - printf-like format string which supports the following specifiers:
  - `%hhd`, `%hd`, `%d`, `%ld`, `%lld` - for `char`, `short`, `int`, `long`, `int64_t`
  - `%hhu`, `%hu`, `%u`, `%lu`, `%llu` - same but for unsigned variants
  - `%hhx`, `%hx`, `%x`, `%lx`, `%llx` - same, for unsigned hex output
  - `%g`, `%f` - for `double`
  - `%c` - for `char`
  - `%s` - for `char *`
  - `%%` - prints `%` character itself
  - `%p` - for any pointer, prints `0x.....` hex value
  - `%M` - (NON-STANDARD EXTENSION): prints using a custom format function
  - `%X.Y` - optional width and precision modifiers
  - `%.*` - optional precision modifier specified as `int` argument

Return value: Number of bytes printed

### m\_snprintf(), m\_vsnprintf()
```c
size_t m_vsnprintf(char *buf, size_t len, const char *fmt, va_list *ap);
size_t m_snprintf(char *buf, size_t len, const char *fmt, ...);
```

Print formatted string into a given buffer. Parameters:
- `buf` - a buffer to print to. Can be NULL, in this case `len` must be 0
- `len` - a size of the `buf`
- `fmt` - a format string. Supports all specifiers mentioned above

Return value: number of bytes printed. The result is guaranteed to be NUL terminated

### Pre-defined `%M` format functions

```c
size_t m_fmt_ip4(void (*out)(char, void *), void *arg, va_list *ap);
size_t m_fmt_ip6(void (*out)(char, void *), void *arg, va_list *ap);
size_t m_fmt_mac(void (*out)(char, void *), void *arg, va_list *ap);
size_t m_fmt_b64(void (*out)(char, void *), void *arg, va_list *ap);
size_t m_fmt_esc(void (*out)(char, void *), void *arg, va_list *ap);
size_t m_fmt_quo(void (*out)(char, void *), void *arg, va_list *ap);
```

Pre-defined helper functions for `%M` specifier:
- `m_fmt_ip4` - prints IPv4 address, expects a pointer to 4-byte IPv4 address
- `m_fmt_ip6` - prints IPv6 address, expects a pointer to 16-byte IPv6 address
- `m_fmt_mac` - prints MAC address, expects a pointer to 6-byte MAC address
- `m_fmt_b64` - prints base64 encoded data, expects `int`, `void *`
- `m_fmt_esc` - prints a string escaping `\n`, `\t`, `\r`, `"`; espects `char *`
- `m_fmt_quo` - double-quote the result of the following %M function

Examples:

```
uint32_t ip4 = 0x0100007f;                              // Print IPv4 address:
m_snprintf(buf, sizeof(buf), "%M", m_fmt_ip4, &ip4);    // 127.0.0.1

uint8_t ip6[16] = {1, 100, 33};                         // Print IPv6 address:
m_snprintf(buf, sizeof(buf), "%M", m_fmt_ip4, &ip6);    // [164:2100:0:0:0:0:0:0]

uint8_t mac[6] = {1, 2, 3, 4, 5, 6};                    // Print MAC address:
m_snprintf(buf, sizeof(buf), "%M", m_fmt_mac, &mac);    // 01:02:03:04:05:06

const char str[] = {'a', \n, '"', 0};                   // Print escaped string:
m_snprintf(buf, sizeof(buf), "%M", m_fmt_esc, str);     // a\n\"

const char *data = "xyz";                               // Print base64 data:
m_snprintf(buf, sizeof(buf), "%M", m_fmt_b64, 3, data); // eHl6

// Double quote the result of the %M formatter, result: "127.0.0.1"
m_snprintf(buf, sizeof(buf), "%M", m_fmt_quo, m_fmt_ip4, &ip4);
```

### Custom `%M` format functions

It is easy to create your own format functions to format data that is
specific to your application. For example, if you want to print your
data structure as JSON string, just create your custom formatting function:

```c
struct foo { int a; double b; const char *c; };

size_t m_fmt_foo(void (*out)(char, void *), void *arg, va_list *ap) {
  struct foo *foo = va_arg(*ap, struct foo *);
  return m_xprintf(out, arg, "{\"a\":%d, \"b\":%g, \"c\":%M}",
                   foo->a, foo->b, m_fmt_quo, m_fmt_esc, c);
}
```

And now, you can use that function:

```
struct foo foo = {1, 2.34, "hi"};
m_snprintf(buf, sizeof(buf), "%M", m_fmt_foo, &foo);
```

## Printing to a dynamic memory

The `m_*printf()` functions always return the total number of bytes that the
result string takes. Therefore it is possible to print to a `malloc()-ed`
buffer in two passes: in the first pass we calculate the length, and in the
second pass we print:

```c
size_t len = m_snprintf(NULL, 0, "Hello, %s", "world");
char *buf = malloc(len + 1);  // +1 is for the NUL terminator
m_snprintf(buf, len + 1, "Hello, %s", "world");
...
free(buf);
```

## Footprint

The following table contains footprint measurements for the ARM Cortex-M0
and ARM Cortex-M7 builds of `m_snprintf()` compared to the standard
`snprintf()`. The compilation is done with `-Os` flag using ARM GCC 10.3.1.
The source code is at `test/footprint.c`, and the corresponding Makefile
snippet is at `test/Makefile`.

|                            | Cortex-M0 | Cortex-M7 |
| -------------------------- | --------- | --------- |
| `m_snprintf` (no float)    | 1844      | 2128      |
| `m_snprintf`               | 10996     | 5592      |
| Standard `snprintf`  (no float)  | 68368     | 68248     |
| Standard `snprintf`        | 87476     | 81420     |

Notes:
- by default, standard snrpintf does not support float, and `m_*printf` does 
- to enable float for ARM GCC (newlib), use `-u _printf_float`
- to disable float for `m_*printf`, use `-DNOFLOAT`

## Licensing

The Micro Printf is licensed under the dual license:
- Open source projects are covered by the GNU Affero 3.0 license
- Commercial projects/products are covered by the commercial,
  permanent, royalty free license
- For licensing questions, [contact us](https://mongoose.ws/contact/)
