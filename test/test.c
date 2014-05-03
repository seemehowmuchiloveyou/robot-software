#include <inttypes.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"
#include "buf.h"

#include <stdbool.h>
#include "cmp.h"

#ifdef __GNUC__
static void error_printf(const char *msg, ...) __attribute__ ((format (printf, 1, 2)));
#else
static void error_printf(const char *msg, ...);
#endif

#define run_tests(t)                                    \
  printf(#t " test: ");                                 \
  if (!run_ ## t ## _tests()) {                         \
    M_BufferSeek(&error_message, 0);                    \
    printf("-- FAILED --\n");                           \
    printf("\t %s\n", M_BufferGetData(&error_message)); \
    return EXIT_FAILURE;                                \
  }                                                     \
  else {                                                \
    printf("passed\n");                                 \
  }

#define test_format(wfunc, otype, in, data, dlen)                             \
  M_BufferClear(&buf);                                                        \
  error_clear();                                                              \
  if (!wfunc(&cmp, in)) {                                                     \
    error_printf("%s(&cmp, %s) failed: %s\n",                                 \
      #wfunc, #in, cmp_strerror(&cmp)                                         \
    );                                                                        \
    return false;                                                             \
  }                                                                           \
  M_BufferSeek(&buf, 0);                                                      \
  if (!M_BufferEqualsData(&buf, data, dlen)) {                                \
    error_printf("%s(&cmp, %s) wrote invalid MessagePack data\n\n",           \
      #wfunc, #in                                                             \
    );                                                                        \
    error_print_bin(data, dlen);                                              \
    error_print_bin(M_BufferGetData(&buf), M_BufferGetSize(&buf));            \
    return false;                                                             \
  }                                                                           \
  M_BufferSeek(&buf, 0);                                                      \
  if (!cmp_read_object(&cmp, &obj)) {                                         \
    error_printf("Error reading object written by %s(&cmp, %s): %s\n",        \
      #wfunc, #in, cmp_strerror(&cmp)                                         \
    );                                                                        \
    return false;                                                             \
  }                                                                           \
  if (obj.as.otype != in) {                                                   \
    error_printf("Input/Output mismatch: %s(&cmp, %s) != ",                   \
      #wfunc, #in                                                             \
    );                                                                        \
    error_print_object(&obj);                                                 \
    error_printf("\n");                                                       \
    return false;                                                             \
  }

#define test_format_with_length(wfunc, otype, in, len, data, dlen)            \
  M_BufferClear(&buf);                                                        \
  error_clear();                                                              \
  if (!wfunc(&cmp, in, len)) {                                                \
    error_printf("%s(&cmp, %s, %d) failed: %s\n",                             \
      #wfunc, #in, len, cmp_strerror(&cmp)                                    \
    );                                                                        \
    return false;                                                             \
  }                                                                           \
  M_BufferSeek(&buf, 0);                                                      \
  if (!M_BufferEqualsData(&buf, data, dlen)) {                                \
    error_printf("%s(&cmp, %s, %d) wrote invalid MessagePack data.\n",        \
      #wfunc, #in, len                                                        \
    );                                                                        \
    error_print_bin(data, dlen);                                              \
    error_print_bin(M_BufferGetData(&buf), M_BufferGetSize(&buf));            \
    return false;                                                             \
  }                                                                           \
  M_BufferSeek(&buf, 0);                                                      \
  if (!cmp_read_object(&cmp, &obj)) {                                         \
    error_printf("Error reading object written by %s(&cmp, %s, %d): %s\n",    \
      #wfunc, #in, len, cmp_strerror(&cmp)                                    \
    );                                                                        \
    return false;                                                             \
  }                                                                           \
  if (obj.as.otype != len) {                                                  \
    error_printf("Input/Output mismatch: %s(&cmp, %s, %d) != ",               \
      #wfunc, #in, len                                                        \
    );                                                                        \
    error_print_object(&obj);                                                 \
    error_printf("\n");                                                       \
    return false;                                                             \
  }

#define test_format_no_input(wfunc, otype, data, dlen, out)                   \
  M_BufferClear(&buf);                                                        \
  error_clear();                                                              \
  if (!wfunc(&cmp)) {                                                         \
    error_printf("%s(&cmp) failed: %s\n", #wfunc, cmp_strerror(&cmp));        \
    return false;                                                             \
  }                                                                           \
  M_BufferSeek(&buf, 0);                                                      \
  if (!M_BufferEqualsData(&buf, data, dlen)) {                                \
    error_printf("%s(&cmp) wrote invalid MessagePack data\n\n", #wfunc);      \
    error_print_bin(data, dlen);                                              \
    error_print_bin(M_BufferGetData(&buf), M_BufferGetSize(&buf));            \
    return false;                                                             \
  }                                                                           \
  M_BufferSeek(&buf, 0);                                                      \
  if (!cmp_read_object(&cmp, &obj)) {                                         \
    error_printf("Error reading object written by %s(&cmp): %s\n",            \
      #wfunc, cmp_strerror(&cmp)                                              \
    );                                                                        \
    return false;                                                             \
  }                                                                           \
  if (obj.as.otype != out) {                                                  \
    error_printf("Input/Output mismatch: %s(&cmp) != %s\n",                   \
      #wfunc, #out                                                            \
    );                                                                        \
    error_print_object(&obj);                                                 \
    error_printf("\n");                                                       \
    return false;                                                             \
  }

#define test_fixext_format(wfunc, etype, esize, in, data, dlen)               \
  M_BufferClear(&buf);                                                        \
  error_clear();                                                              \
  if (!wfunc(&cmp, etype, in)) {                                              \
    error_printf("%s(&cmp, %d, %s) failed: %s\n",                             \
      #wfunc, etype, in, cmp_strerror(&cmp)                                   \
    );                                                                        \
    return false;                                                             \
  }                                                                           \
  M_BufferSeek(&buf, 0);                                                      \
  if (!M_BufferEqualsData(&buf, data, dlen)) {                                \
    error_printf("%s(&cmp, %d, %s) wrote invalid MessagePack data.\n",        \
      #wfunc, etype, in                                                       \
    );                                                                        \
    error_print_bin(data, dlen);                                              \
    error_print_bin(M_BufferGetData(&buf), M_BufferGetSize(&buf));            \
    return false;                                                             \
  }                                                                           \
  M_BufferSeek(&buf, 0);                                                      \
  if (!cmp_read_object(&cmp, &obj)) {                                         \
    error_printf("Error reading object written by %s(&cmp, %d, %s): %s\n",    \
      #wfunc, etype, in, cmp_strerror(&cmp)                                   \
    );                                                                        \
    return false;                                                             \
  }                                                                           \
  if (obj.as.ext.type != etype || obj.as.ext.size != esize) {                 \
    error_printf(                                                             \
      "Input/Output mismatch: %s(&cmp, %d, %s) != {%d, %u}\n",                \
      #wfunc, etype, in, obj.as.ext.type, obj.as.ext.size                     \
    );                                                                        \
    return false;                                                             \
  }

#define test_ext_format(wfunc, etype, esize, in, data, dlen)                  \
  M_BufferClear(&buf);                                                        \
  error_clear();                                                              \
  if (!wfunc(&cmp, etype, esize, in)) {                                       \
    error_printf("%s(&cmp, %d, %u, %s) failed: %s\n",                         \
      #wfunc, etype, esize, in, cmp_strerror(&cmp)                            \
    );                                                                        \
    return false;                                                             \
  }                                                                           \
  M_BufferSeek(&buf, 0);                                                      \
  if (!M_BufferEqualsData(&buf, data, dlen)) {                                \
    error_printf("%s(&cmp, %d, %u, %s) wrote invalid MessagePack data.\n",    \
      #wfunc, etype, esize, in                                                \
    );                                                                        \
    error_print_bin(data, dlen);                                              \
    error_print_bin(M_BufferGetData(&buf), M_BufferGetSize(&buf));            \
    return false;                                                             \
  }                                                                           \
  M_BufferSeek(&buf, 0);                                                      \
  if (!cmp_read_object(&cmp, &obj)) {                                         \
    error_printf(                                                             \
      "Error reading object written by %s(&cmp, %d, %u, %s): %s\n",           \
      #wfunc, etype, esize, in, cmp_strerror(&cmp)                            \
    );                                                                        \
    return false;                                                             \
  }                                                                           \
  if (obj.as.ext.type != etype || obj.as.ext.size != esize) {                 \
    error_printf(                                                             \
      "Input/Output mismatch: %s(&cmp, %d, %u, %s) != {%d, %u}\n",            \
      #wfunc, etype, esize, in, obj.as.ext.type, obj.as.ext.size              \
    );                                                                        \
    return false;                                                             \
  }

static buf_t error_message;

static bool buf_reader(cmp_ctx_t *ctx, void *data, size_t limit) {
  buf_t *buf = (buf_t *)ctx->buf;

  return M_BufferRead(buf, data, limit);
}

static size_t buf_writer(cmp_ctx_t *ctx, const void *data, size_t sz) {
  buf_t *buf = (buf_t *)ctx->buf;
  size_t pos = M_BufferGetCursor(buf);

  M_BufferWrite(buf, (void *)data, sz);

  return M_BufferGetCursor(buf) - pos;
}

static void setup_cmp_and_buf(cmp_ctx_t *cmp, buf_t *buf) {
  M_BufferInitWithCapacity(buf, 32);
  cmp_init(cmp, buf, &buf_reader, &buf_writer);
}

static void error_clear(void) {
  M_BufferClear(&error_message);
}

static void error_printf(const char *msg, ...) {
  size_t len = 0;
  va_list args, print_args, len_test_args;

  va_start(args, msg);

  va_copy(len_test_args, args);
  va_copy(print_args, args);
  len = vsnprintf(NULL, 0, msg, len_test_args) + 1;
  M_BufferEnsureCapacity(&error_message, len);

  vfprintf(stderr, msg, print_args);

  vsnprintf(M_BufferGetData(&error_message), len, msg, args);

  va_end(args);
}

static void error_print_bin(const char *data, size_t length) {
  size_t i;

  for (i = 0; i < length; i++) {
    error_printf("%02x ", data[i]);

    if ((i != 0) && ((i % 26) == 0))
      error_printf("\n");
  }
  error_printf("\n");
}

static void error_print_object(cmp_object_t *obj) {
  switch (obj->type) {
    case CMP_TYPE_POSITIVE_FIXNUM:
    case CMP_TYPE_UINT8:
      error_printf("%u", obj->as.u8);
      break;
    case CMP_TYPE_FIXMAP:
    case CMP_TYPE_MAP16:
    case CMP_TYPE_MAP32:
    case CMP_TYPE_FIXARRAY:
    case CMP_TYPE_ARRAY16:
    case CMP_TYPE_ARRAY32:
    case CMP_TYPE_FIXSTR:
    case CMP_TYPE_STR8:
    case CMP_TYPE_STR16:
    case CMP_TYPE_STR32:
    case CMP_TYPE_BIN8:
    case CMP_TYPE_BIN16:
    case CMP_TYPE_BIN32:
      error_printf("%u", obj->as.bin_size);
      break;
    case CMP_TYPE_NIL:
      error_printf("NULL");
      break;
    case CMP_TYPE_BOOLEAN:
      if (obj->as.boolean)
        error_printf("true");
      else
        error_printf("false");
      break;
    case CMP_TYPE_EXT8:
    case CMP_TYPE_EXT16:
    case CMP_TYPE_EXT32:
    case CMP_TYPE_FIXEXT1:
    case CMP_TYPE_FIXEXT2:
    case CMP_TYPE_FIXEXT4:
    case CMP_TYPE_FIXEXT8:
    case CMP_TYPE_FIXEXT16:
      error_printf("{%d, %u}", obj->as.ext.type, obj->as.ext.size);
      break;
    case CMP_TYPE_FLOAT:
      error_printf("%f", obj->as.flt);
      break;
    case CMP_TYPE_DOUBLE:
      error_printf("%f", obj->as.dbl);
      break;
    case CMP_TYPE_UINT16:
      error_printf("%u", obj->as.u16);
      break;
    case CMP_TYPE_UINT32:
      error_printf("%u", obj->as.u32);
      break;
    case CMP_TYPE_UINT64:
      error_printf("%" PRIu64, obj->as.u64);
      break;
    case CMP_TYPE_NEGATIVE_FIXNUM:
    case CMP_TYPE_SINT8:
      error_printf("%d", obj->as.s8);
      break;
    case CMP_TYPE_SINT16:
      error_printf("%d", obj->as.s16);
      break;
    case CMP_TYPE_SINT32:
      error_printf("%d", obj->as.s32);
      break;
    case CMP_TYPE_SINT64:
      error_printf("%" PRId64, obj->as.s64);
      break;
  }
}

bool run_msgpack_tests(void) {
  buf_t in_buf, out_buf;
  cmp_ctx_t in_cmp, out_cmp;
  cmp_object_t obj;
  dboolean buffers_equal = false;

  setup_cmp_and_buf(&in_cmp, &in_buf);
  M_BufferSetFile(&in_buf, "cases.mpac");
  M_BufferSeek(&in_buf, 0);

  setup_cmp_and_buf(&out_cmp, &out_buf);
  M_BufferEnsureCapacity(&out_buf, M_BufferGetSize(&in_buf));

  while (M_BufferGetCursor(&in_buf) < (M_BufferGetSize(&in_buf))) {
    if (!cmp_read_object(&in_cmp, &obj)) {
      error_printf("Error reading object: %s\n", cmp_strerror(&in_cmp));
      return false;
    }
    if (!cmp_write_object(&out_cmp, &obj)) {
      error_printf("Error writing object: %s\n", cmp_strerror(&out_cmp));
      return false;
    }
  }

  M_BufferSeek(&in_buf, 0);
  M_BufferSeek(&out_buf, 0);
  buffers_equal = M_BufferEqualsData(
    &in_buf, M_BufferGetData(&out_buf), M_BufferGetSize(&out_buf)
  );

  if (!buffers_equal) {
    error_printf("Buffers did not match.\n");
    M_BufferPrint(&in_buf);
    M_BufferPrint(&out_buf);
    M_BufferFree(&in_buf);
    M_BufferFree(&out_buf);

    return false;
  }

  M_BufferFree(&in_buf);
  M_BufferFree(&out_buf);

  return true;
}

bool run_fixedint_tests(void) {
  buf_t buf;
  cmp_ctx_t cmp;
  cmp_object_t obj;

  setup_cmp_and_buf(&cmp, &buf);

  error_clear();

  if (cmp_write_pfix(&cmp, 128)) {
    error_printf("Wrote a positive fixed integer that was too large.\n");
    return false;
  }
  cmp.error = 0;

  if (cmp_write_pfix(&cmp, 200)) {
    error_printf("Wrote a positive fixed integer that was too large.\n");
    return false;
  }
  cmp.error = 0;

  if (cmp_write_pfix(&cmp, -1)) {
    error_printf("Wrote a negative positive fixed integer (-1).\n");
    return false;
  }
  cmp.error = 0;

  if (cmp_write_pfix(&cmp, -31)) {
    error_printf("Wrote a negative positive fixed integer (-31).\n");
    return false;
  }
  cmp.error = 0;

  if (cmp_write_pfix(&cmp, -32)) {
    error_printf("Wrote a negative positive fixed integer (-32).\n");
    return false;
  }
  cmp.error = 0;

  if (cmp_write_pfix(&cmp, -127)) {
    error_printf("Wrote a negative positive fixed integer (-127).\n");
    return false;
  }
  cmp.error = 0;

  if (cmp_write_pfix(&cmp, -128)) {
    error_printf("Wrote a negative positive fixed integer (-128).\n");
    return false;
  }
  cmp.error = 0;

  if (cmp_write_ufix(&cmp, -128)) {
    error_printf("Wrote a negative unsigned fixed integer (-128).\n");
    return false;
  }
  cmp.error = 0;

  if (cmp_write_ufix(&cmp, -1)) {
    error_printf("Wrote a negative unsigned fixed integer.\n");
    return false;
  }
  cmp.error = 0;

  if (cmp_write_ufix(&cmp, -128)) {
    error_printf("Wrote a negative unsigned fixed integer.\n");
    return false;
  }
  cmp.error = 0;

  if (cmp_write_sfix(&cmp, -33)) {
    error_printf("Wrote a negative signed fixed integer that was too large.\n");
    return false;
  }
  cmp.error = 0;

  if (cmp_write_nfix(&cmp, 0)) {
    error_printf("Wrote 0 as a negative fixed integer.\n");
    return false;
  }
  cmp.error = 0;

  if (cmp_write_nfix(&cmp, 1)) {
    error_printf("Wrote 1 as a negative fixed integer.\n");
    return false;
  }
  cmp.error = 0;

  if (cmp_write_nfix(&cmp, -33)) {
    error_printf("Wrote a negative fixed integer that was too large.\n");
    return false;
  }
  cmp.error = 0;

  test_format(cmp_write_ufix, u8, 0, "\x00", 1);
  test_format(cmp_write_ufix, u8, -0, "\x00", 1);
  test_format(cmp_write_sfix, u8, 0, "\x00", 1);
  test_format(cmp_write_sfix, s8, -0, "\x00", 1);
  test_format(cmp_write_sfix, u8, 127, "\x7f", 1);
  test_format(cmp_write_sfix, s8, -32, "\xe0", 1);
  test_format(cmp_write_pfix, u8, 0, "\x00", 1);
  test_format(cmp_write_pfix, u8, 1, "\x01", 1);
  test_format(cmp_write_pfix, u8, 127, "\x7f", 1);
  test_format(cmp_write_nfix, s8, -1, "\xff", 1);
  test_format(cmp_write_nfix, s8, -32, "\xe0", 1);

  return true;
}

bool run_number_tests(void) {
  buf_t buf;
  cmp_ctx_t cmp;
  cmp_object_t obj;

  setup_cmp_and_buf(&cmp, &buf);

  test_format(cmp_write_s8, s8,  0,   "\xd0\x00", 2);
  test_format(cmp_write_s8, s8,  1,   "\xd0\x01", 2);
  test_format(cmp_write_s8, s8, -1,   "\xd0\xff", 2);
  test_format(cmp_write_s8, s8,  127, "\xd0\x7f", 2);
  test_format(cmp_write_s8, s8, -128, "\xd0\x80", 2);

  test_format(cmp_write_s16, s16,  0,     "\xd1\x00\x00", 3);
  test_format(cmp_write_s16, s16,  1,     "\xd1\x00\x01", 3);
  test_format(cmp_write_s16, s16, -1,     "\xd1\xff\xff", 3);
  test_format(cmp_write_s16, s16,  127,   "\xd1\x00\x7f", 3);
  test_format(cmp_write_s16, s16, -128,   "\xd1\xff\x80", 3);
  test_format(cmp_write_s16, s16,  256,   "\xd1\x01\x00", 3);
  test_format(cmp_write_s16, s16,  32767, "\xd1\x7f\xff", 3);
  test_format(cmp_write_s16, s16, -32768, "\xd1\x80\x00", 3);

  test_format(cmp_write_s32, s32,  0,          "\xd2\x00\x00\x00\x00", 5);
  test_format(cmp_write_s32, s32,  1,          "\xd2\x00\x00\x00\x01", 5);
  test_format(cmp_write_s32, s32, -1,          "\xd2\xff\xff\xff\xff", 5);
  test_format(cmp_write_s32, s32,  127,        "\xd2\x00\x00\x00\x7f", 5);
  test_format(cmp_write_s32, s32, -128,        "\xd2\xff\xff\xff\x80", 5);
  test_format(cmp_write_s32, s32,  256,        "\xd2\x00\x00\x01\x00", 5);
  test_format(cmp_write_s32, s32,  32767,      "\xd2\x00\x00\x7f\xff", 5);
  test_format(cmp_write_s32, s32, -32768,      "\xd2\xff\xff\x80\x00", 5);
  test_format(cmp_write_s32, s32,  65535,      "\xd2\x00\x00\xff\xff", 5);
  test_format(cmp_write_s32, s32, -65536,      "\xd2\xff\xff\x00\x00", 5);
  test_format(cmp_write_s32, s32,  8388607,    "\xd2\x00\x7f\xff\xff", 5);
  test_format(cmp_write_s32, s32, -8388608,    "\xd2\xff\x80\x00\x00", 5);
  test_format(cmp_write_s32, s32,  16777215,   "\xd2\x00\xff\xff\xff", 5);
  test_format(cmp_write_s32, s32, -16777216,   "\xd2\xff\x00\x00\x00", 5);
  test_format(cmp_write_s32, s32,  2147483647, "\xd2\x7f\xff\xff\xff", 5);
  test_format(cmp_write_s32, s32, -2147483648, "\xd2\x80\x00\x00\x00", 5);

  test_format(cmp_write_s64, s64,  0,          "\xd3\x00\x00\x00\x00\x00\x00\x00\x00", 9);
  test_format(cmp_write_s64, s64,  1,          "\xd3\x00\x00\x00\x00\x00\x00\x00\x01", 9);
  test_format(cmp_write_s64, s64, -1,          "\xd3\xff\xff\xff\xff\xff\xff\xff\xff", 9);
  test_format(cmp_write_s64, s64,  127,        "\xd3\x00\x00\x00\x00\x00\x00\x00\x7f", 9);
  test_format(cmp_write_s64, s64, -128,        "\xd3\xff\xff\xff\xff\xff\xff\xff\x80", 9);
  test_format(cmp_write_s64, s64,  256,        "\xd3\x00\x00\x00\x00\x00\x00\x01\x00", 9);
  test_format(cmp_write_s64, s64,  32767,      "\xd3\x00\x00\x00\x00\x00\x00\x7f\xff", 9);
  test_format(cmp_write_s64, s64, -32768,      "\xd3\xff\xff\xff\xff\xff\xff\x80\x00", 9);
  test_format(cmp_write_s64, s64,  65535,      "\xd3\x00\x00\x00\x00\x00\x00\xff\xff", 9);
  test_format(cmp_write_s64, s64, -65536,      "\xd3\xff\xff\xff\xff\xff\xff\x00\x00", 9);
  test_format(cmp_write_s64, s64,  8388607,    "\xd3\x00\x00\x00\x00\x00\x7f\xff\xff", 9);
  test_format(cmp_write_s64, s64, -8388608,    "\xd3\xff\xff\xff\xff\xff\x80\x00\x00", 9);
  test_format(cmp_write_s64, s64,  16777215,   "\xd3\x00\x00\x00\x00\x00\xff\xff\xff", 9);
  test_format(cmp_write_s64, s64, -16777216,   "\xd3\xff\xff\xff\xff\xff\x00\x00\x00", 9);
  test_format(cmp_write_s64, s64,  2147483647, "\xd3\x00\x00\x00\x00\x7f\xff\xff\xff", 9);
  test_format(cmp_write_s64, s64, -2147483648, "\xd3\xff\xff\xff\xff\x80\x00\x00\x00", 9);
  test_format(cmp_write_s64, s64,  4294967295, "\xd3\x00\x00\x00\x00\xff\xff\xff\xff", 9);
  test_format(cmp_write_s64, s64, -4294967296, "\xd3\xff\xff\xff\xff\x00\x00\x00\x00", 9);

  test_format(cmp_write_sint, u8,  0,           "\x00", 1);
  test_format(cmp_write_sint, u8,  1,           "\x01", 1);
  test_format(cmp_write_sint, u8,  127,         "\x7f", 1);
  test_format(cmp_write_sint, u8,  128,         "\xcc\x80", 2);
  test_format(cmp_write_sint, u8,  255,         "\xcc\xff", 2);
  test_format(cmp_write_sint, u16, 256,         "\xcd\x01\x00", 3);
  test_format(cmp_write_sint, u16, 32767,       "\xcd\x7f\xff", 3);
  test_format(cmp_write_sint, u16, 32768,       "\xcd\x80\x00", 3);
  test_format(cmp_write_sint, u16, 65535,       "\xcd\xff\xff", 3);
  test_format(cmp_write_sint, u32, 65536,       "\xce\x00\x01\x00\x00", 5);
  test_format(cmp_write_sint, u32, 8388607,     "\xce\x00\x7f\xff\xff", 5);
  test_format(cmp_write_sint, u32, 8388608,     "\xce\x00\x80\x00\x00", 5);
  test_format(cmp_write_sint, u32, 16777215,    "\xce\x00\xff\xff\xff", 5);
  test_format(cmp_write_sint, u32, 16777216,    "\xce\x01\x00\x00\x00", 5);
  test_format(cmp_write_sint, u32, 2147483647,  "\xce\x7f\xff\xff\xff", 5);
  test_format(
    cmp_write_sint, u64, 4294967296, "\xcf\x00\x00\x00\x01\x00\x00\x00\x00", 9
  );
  test_format(
    cmp_write_sint, u64, 549755813887, "\xcf\x00\x00\x00\x7f\xff\xff\xff\xff", 9
  );
  test_format(
    cmp_write_sint, u64, 549755813888, "\xcf\x00\x00\x00\x80\x00\x00\x00\x00", 9
  );
  test_format(
    cmp_write_sint, u64, 1099511627775, "\xcf\x00\x00\x00\xff\xff\xff\xff\xff", 9
  );
  test_format(
    cmp_write_sint, u64, 1099511627776, "\xcf\x00\x00\x01\x00\x00\x00\x00\x00", 9
  );
  test_format(
    cmp_write_sint, u64, 140737488355327, "\xcf\x00\x00\x7f\xff\xff\xff\xff\xff", 9
  );
  test_format(
    cmp_write_sint, u64, 140737488355328, "\xcf\x00\x00\x80\x00\x00\x00\x00\x00", 9
  );
  test_format(
    cmp_write_sint, u64, 281474976710655, "\xcf\x00\x00\xff\xff\xff\xff\xff\xff", 9
  );
  test_format(
    cmp_write_sint, u64, 281474976710656, "\xcf\x00\x01\x00\x00\x00\x00\x00\x00", 9
  );
  test_format(
    cmp_write_sint, u64, 36028797018963967, "\xcf\x00\x7f\xff\xff\xff\xff\xff\xff", 9
  );
  test_format(
    cmp_write_sint, u64, 36028797018963968, "\xcf\x00\x80\x00\x00\x00\x00\x00\x00", 9
  );
  test_format(
    cmp_write_sint, u64, 72057594037927935, "\xcf\x00\xff\xff\xff\xff\xff\xff\xff", 9
  );
  test_format(
    cmp_write_sint, u64, 72057594037927936, "\xcf\x01\x00\x00\x00\x00\x00\x00\x00", 9
  );
  test_format(
    cmp_write_sint, u64, 9223372036854775807, "\xcf\x7f\xff\xff\xff\xff\xff\xff\xff", 9
  );

  test_format(cmp_write_sint, s8,  -1,          "\xff", 1);
  test_format(cmp_write_sint, s8,  -32,         "\xe0", 1);
  test_format(cmp_write_sint, s8,  -127,        "\xd0\x81", 2);
  test_format(cmp_write_sint, s8,  -128,        "\xd0\x80", 2);
  test_format(cmp_write_sint, s16, -255,        "\xd1\xff\x01", 3);
  test_format(cmp_write_sint, s16, -256,        "\xd1\xff\x00", 3);
  test_format(cmp_write_sint, s16, -32767,      "\xd1\x80\x01", 3);
  test_format(cmp_write_sint, s16, -32768,      "\xd1\x80\x00", 3);
  test_format(cmp_write_sint, s32, -65535,      "\xd2\xff\xff\x00\x01", 5);
  test_format(cmp_write_sint, s32, -65536,      "\xd2\xff\xff\x00\x00", 5);
  test_format(cmp_write_sint, s32, -8388607,    "\xd2\xff\x80\x00\x01", 5);
  test_format(cmp_write_sint, s32, -8388608,    "\xd2\xff\x80\x00\x00", 5);
  test_format(cmp_write_sint, s32, -16777215,   "\xd2\xff\x00\x00\x01", 5);
  test_format(cmp_write_sint, s32, -16777216,   "\xd2\xff\x00\x00\x00", 5);
  test_format(cmp_write_sint, s32, -2147483647, "\xd2\x80\x00\x00\x01", 5);
  test_format(cmp_write_sint, s32, -2147483648, "\xd2\x80\x00\x00\x00", 5);
  test_format(
    cmp_write_sint, s64, -4294967295, "\xd3\xff\xff\xff\xff\x00\x00\x00\x01", 9
  );
  test_format(
    cmp_write_sint, s64, -4294967296, "\xd3\xff\xff\xff\xff\x00\x00\x00\x00", 9
  );
  test_format(
    cmp_write_sint, s64, -549755813887, "\xd3\xff\xff\xff\x80\x00\x00\x00\x01", 9
  );
  test_format(
    cmp_write_sint, s64, -549755813888, "\xd3\xff\xff\xff\x80\x00\x00\x00\x00", 9
  );
  test_format(
    cmp_write_sint, s64, -1099511627775, "\xd3\xff\xff\xff\x00\x00\x00\x00\x01", 9
  );
  test_format(
    cmp_write_sint, s64, -1099511627776, "\xd3\xff\xff\xff\x00\x00\x00\x00\x00", 9
  );
  test_format(
    cmp_write_sint, s64, -140737488355327, "\xd3\xff\xff\x80\x00\x00\x00\x00\x01", 9
  );
  test_format(
    cmp_write_sint, s64, -140737488355328, "\xd3\xff\xff\x80\x00\x00\x00\x00\x00", 9
  );
  test_format(
    cmp_write_sint, s64, -281474976710655, "\xd3\xff\xff\x00\x00\x00\x00\x00\x01", 9
  );
  test_format(
    cmp_write_sint, s64, -281474976710656, "\xd3\xff\xff\x00\x00\x00\x00\x00\x00", 9
  );
  test_format(
    cmp_write_sint, s64, -36028797018963967, "\xd3\xff\x80\x00\x00\x00\x00\x00\x01", 9
  );
  test_format(
    cmp_write_sint, s64, -36028797018963968, "\xd3\xff\x80\x00\x00\x00\x00\x00\x00", 9
  );
  test_format(
    cmp_write_sint, s64, -72057594037927935, "\xd3\xff\x00\x00\x00\x00\x00\x00\x01", 9
  );
  test_format(
    cmp_write_sint, s64, -72057594037927936, "\xd3\xff\x00\x00\x00\x00\x00\x00\x00", 9
  );
  test_format(
    cmp_write_sint, s64, -9223372036854775807, "\xd3\x80\x00\x00\x00\x00\x00\x00\x01", 9
  );

  test_format(cmp_write_u8, u8, 0,   "\xcc\x00", 1);
  test_format(cmp_write_u8, u8, 1,   "\xcc\x01", 1);
  test_format(cmp_write_u8, u8, 127, "\xcc\x7f", 1);
  test_format(cmp_write_u8, u8, 255, "\xcc\xff", 1);

  test_format(cmp_write_u16, u16, 0,     "\xcd\x00\x00", 2);
  test_format(cmp_write_u16, u16, 1,     "\xcd\x00\x01", 2);
  test_format(cmp_write_u16, u16, 127,   "\xcd\x00\x7f", 2);
  test_format(cmp_write_u16, u16, 256,   "\xcd\x01\x00", 2);
  test_format(cmp_write_u16, u16, 32767, "\xcd\x7f\xff", 2);
  test_format(cmp_write_u16, u16, 65535, "\xcd\xff\xff", 2);

  test_format(cmp_write_u32, u32, 0,          "\xce\x00\x00\x00\x00", 5);
  test_format(cmp_write_u32, u32, 1,          "\xce\x00\x00\x00\x01", 5);
  test_format(cmp_write_u32, u32, 127,        "\xce\x00\x00\x00\x7f", 5);
  test_format(cmp_write_u32, u32, 256,        "\xce\x00\x00\x01\x00", 5);
  test_format(cmp_write_u32, u32, 32767,      "\xce\x00\x00\x7f\xff", 5);
  test_format(cmp_write_u32, u32, 65535,      "\xce\x00\x00\xff\xff", 5);
  test_format(cmp_write_u32, u32, 8388607,    "\xce\x00\x7f\xff\xff", 5);
  test_format(cmp_write_u32, u32, 16777215,   "\xce\x00\xff\xff\xff", 5);
  test_format(cmp_write_u32, u32, 2147483647, "\xce\x7f\xff\xff\xff", 5);
  test_format(cmp_write_u32, u32, 4294967295, "\xce\xff\xff\xff\xff", 5);

  test_format(cmp_write_u64, u64, 0, "\xcf\x00\x00\x00\x00\x00\x00\x00\x00", 9);
  test_format(cmp_write_u64, u64, 1, "\xcf\x00\x00\x00\x00\x00\x00\x00\x01", 9);
  test_format(cmp_write_u64, u64, 127, "\xcf\x00\x00\x00\x00\x00\x00\x00\x7f", 9);
  test_format(cmp_write_u64, u64, 256, "\xcf\x00\x00\x00\x00\x00\x00\x01\x00", 9);
  test_format(cmp_write_u64, u64, 32767, "\xcf\x00\x00\x00\x00\x00\x00\x7f\xff", 9);
  test_format(cmp_write_u64, u64, 65535, "\xcf\x00\x00\x00\x00\x00\x00\xff\xff", 9);
  test_format(
    cmp_write_u64, u64, 8388607, "\xcf\x00\x00\x00\x00\x00\x7f\xff\xff", 9
  );
  test_format(
    cmp_write_u64, u64, 16777215, "\xcf\x00\x00\x00\x00\x00\xff\xff\xff", 9
  );
  test_format(
    cmp_write_u64, u64, 2147483647, "\xcf\x00\x00\x00\x00\x7f\xff\xff\xff", 9
  );
  test_format(
    cmp_write_u64, u64, 4294967295, "\xcf\x00\x00\x00\x00\xff\xff\xff\xff", 9
  );
  test_format(
    cmp_write_u64, u64,
    0xFFFFFFFFFFFFFFFE,
    "\xcf\xff\xff\xff\xff\xff\xff\xff\xfe",
    9
  );
  test_format(
    cmp_write_u64, u64,
    0xFFFFFFFFFFFFFFFF,
    "\xcf\xff\xff\xff\xff\xff\xff\xff\xff",
    9
  );

  test_format(
    cmp_write_uint, u8, 0, "\x00", 1
  );
  test_format(
    cmp_write_uint, u8, 1, "\x01", 1
  );
  test_format(
    cmp_write_uint, u8, 127, "\x7f", 1
  );
  test_format(
    cmp_write_uint, u8, 128, "\xcc\x80", 2
  );
  test_format(
    cmp_write_uint, u8, 255, "\xcc\xff", 2
  );
  test_format(
    cmp_write_uint, u16, 256, "\xcd\x01\x00", 3
  );
  test_format(
    cmp_write_uint, u16, 32767, "\xcd\x7f\xff", 3
  );
  test_format(
    cmp_write_uint, u16, 32768, "\xcd\x80\x00", 3
  );
  test_format(
    cmp_write_uint, u16, 65535, "\xcd\xff\xff", 3
  );
  test_format(
    cmp_write_uint, u32, 65536, "\xce\x00\x01\x00\x00", 5
  );
  test_format(
    cmp_write_uint, u32, 8388607, "\xce\x00\x7f\xff\xff", 5
  );
  test_format(
    cmp_write_uint, u32, 8388608, "\xce\x00\x80\x00\x00", 5
  );
  test_format(
    cmp_write_uint, u32, 16777215, "\xce\x00\xff\xff\xff", 5
  );
  test_format(
    cmp_write_uint, u32, 16777216, "\xce\x01\x00\x00\x00", 5
  );
  test_format(
    cmp_write_uint, u32, 2147483647, "\xce\x7f\xff\xff\xff", 5
  );
  test_format(
    cmp_write_uint, u32, 2147483648, "\xce\x80\x00\x00\x00", 5
  );
  test_format(
    cmp_write_uint, u32, 4294967295, "\xce\xff\xff\xff\xff", 5
  );
  test_format(
    cmp_write_uint, u64, 4294967296, "\xcf\x00\x00\x00\x01\x00\x00\x00\x00", 9
  );
  test_format(
    cmp_write_uint, u64, 549755813887, "\xcf\x00\x00\x00\x7f\xff\xff\xff\xff", 9
  );
  test_format(
    cmp_write_uint, u64, 549755813888, "\xcf\x00\x00\x00\x80\x00\x00\x00\x00", 9
  );
  test_format(
    cmp_write_uint, u64, 1099511627775, "\xcf\x00\x00\x00\xff\xff\xff\xff\xff", 9
  );
  test_format(
    cmp_write_uint, u64, 1099511627776, "\xcf\x00\x00\x01\x00\x00\x00\x00\x00", 9
  );
  test_format(
    cmp_write_uint, u64, 140737488355327, "\xcf\x00\x00\x7f\xff\xff\xff\xff\xff", 9
  );
  test_format(
    cmp_write_uint, u64, 140737488355328, "\xcf\x00\x00\x80\x00\x00\x00\x00\x00", 9
  );
  test_format(
    cmp_write_uint, u64, 281474976710655, "\xcf\x00\x00\xff\xff\xff\xff\xff\xff", 9
  );
  test_format(
    cmp_write_uint, u64, 281474976710656, "\xcf\x00\x01\x00\x00\x00\x00\x00\x00", 9
  );
  test_format(
    cmp_write_uint, u64, 36028797018963967, "\xcf\x00\x7f\xff\xff\xff\xff\xff\xff", 9
  );
  test_format(
    cmp_write_uint, u64, 36028797018963968, "\xcf\x00\x80\x00\x00\x00\x00\x00\x00", 9
  );
  test_format(
    cmp_write_uint, u64, 72057594037927935, "\xcf\x00\xff\xff\xff\xff\xff\xff\xff", 9
  );
  test_format(
    cmp_write_uint, u64, 72057594037927936, "\xcf\x01\x00\x00\x00\x00\x00\x00\x00", 9
  );
  test_format(
    cmp_write_uint, u64, 9223372036854775807, "\xcf\x7f\xff\xff\xff\xff\xff\xff\xff", 9
  );
  test_format(
    cmp_write_uint, u64, 0xFFFFFFFFFFFFFFFF, "\xcf\xff\xff\xff\xff\xff\xff\xff\xff", 9
  );

  test_format(cmp_write_float, flt, 0.0f,      "\xca\x00\x00\x00\x00", 5);
  test_format(cmp_write_float, flt, -0.0f,     "\xca\x80\x00\x00\x00", 5);
  test_format(cmp_write_float, flt, 1.0f,      "\xca\x3f\x80\x00\x00", 5);
  test_format(cmp_write_float, flt, -1.0f,     "\xca\xbf\x80\x00\x00", 5);
  test_format(cmp_write_float, flt, 65535.0f,  "\xca\x47\x7f\xff\x00", 5);
  test_format(cmp_write_float, flt, -65535.0f, "\xca\xc7\x7f\xff\x00", 5);
  test_format(cmp_write_float, flt, 32767.0f,  "\xca\x46\xff\xfe\x00", 5);
  test_format(cmp_write_float, flt, -32767.0f, "\xca\xc6\xff\xfe\x00", 5);

  test_format(
    cmp_write_double, dbl, 0.0, "\xcb\x00\x00\x00\x00\x00\x00\x00\x00", 9
  );
  test_format(
    cmp_write_double, dbl, -0.0, "\xcb\x80\x00\x00\x00\x00\x00\x00\x00", 9
  );
  test_format(
    cmp_write_double, dbl, 1.0, "\xcb\x3f\xf0\x00\x00\x00\x00\x00\x00", 9
  );
  test_format(
    cmp_write_double, dbl, -1.0, "\xcb\xbf\xf0\x00\x00\x00\x00\x00\x00", 9
  );
  test_format(
    cmp_write_double, dbl, 2147483647.0, "\xcb\x41\xdf\xff\xff\xff\xc0\x00\x00", 9
  );
  test_format(
    cmp_write_double, dbl, -2147483647.0, "\xcb\xc1\xdf\xff\xff\xff\xc0\x00\x00", 9
  );
  test_format(
    cmp_write_double, dbl, 4294967295.0, "\xcb\x41\xef\xff\xff\xff\xe0\x00\x00", 9
  );
  test_format(
    cmp_write_double, dbl, -4294967295.0, "\xcb\xc1\xef\xff\xff\xff\xe0\x00\x00", 9
  );

  return true;
}

bool run_nil_tests(void) {
  buf_t buf;
  cmp_ctx_t cmp;
  cmp_object_t obj;

  setup_cmp_and_buf(&cmp, &buf);

  test_format_no_input(cmp_write_nil, u8, "\xc0", 1, 0)

  return true;
}

bool run_boolean_tests(void) {
  buf_t buf;
  cmp_ctx_t cmp;
  cmp_object_t obj;

  setup_cmp_and_buf(&cmp, &buf);

  test_format_no_input(cmp_write_false, boolean, "\xc2", 1, false)
  test_format_no_input(cmp_write_true,  boolean, "\xc3", 1, true)
  test_format(cmp_write_bool, boolean, false, "\xc2", 1);
  test_format(cmp_write_bool, boolean, true, "\xc3", 1);
  test_format(cmp_write_u8_as_bool, boolean, 0, "\xc2", 1);
  test_format(cmp_write_u8_as_bool, boolean, 1, "\xc3", 1);

  return true;
}

bool run_binary_tests(void) {
  buf_t buf;
  cmp_ctx_t cmp;
  cmp_object_t obj;

  setup_cmp_and_buf(&cmp, &buf);

  test_format_with_length(
    cmp_write_bin8, bin_size, "Hey there\n", 10, "\xc4\x0aHey there\n", 12
  );
  test_format_with_length(
    cmp_write_bin16, bin_size, "Hey there\n", 10, "\xc5\x00\x0aHey there\n", 13
  );
  test_format_with_length(
    cmp_write_bin32, bin_size, "Hey there\n", 10, "\xc6\x00\x00\x00\x0aHey there\n", 15
  );
  test_format_with_length(
    cmp_write_bin, bin_size, "Hey there\n", 10, "\xc4\x0aHey there\n", 12
  );

  return true;
}

bool run_string_tests(void) {
  buf_t buf;
  cmp_ctx_t cmp;
  cmp_object_t obj;

  setup_cmp_and_buf(&cmp, &buf);

  test_format_with_length(
    cmp_write_fixstr, str_size, "Hey there\n", 10, "\xaaHey there\n", 11
  );
  test_format_with_length(
    cmp_write_str8, str_size, "Hey there\n", 10, "\xd9\x0aHey there\n", 12
  );
  test_format_with_length(
    cmp_write_str16, str_size, "Hey there\n", 10, "\xda\x00\x0aHey there\n", 13
  );
  test_format_with_length(
    cmp_write_str32, str_size, "Hey there\n", 10, "\xdb\x00\x00\x00\x0aHey there\n", 15
  );
  test_format_with_length(
    cmp_write_str, str_size, "Hey there\n", 10, "\xaaHey there\n", 11
  );

  return true;
}

bool run_array_tests(void) {
  buf_t buf;
  cmp_ctx_t cmp;
  cmp_object_t obj;

  setup_cmp_and_buf(&cmp, &buf);

  test_format(cmp_write_fixarray, array_size, 10, "\x9a", 1);
  test_format(cmp_write_array16, array_size, 10, "\xdc\x00\x0a", 3);
  test_format(cmp_write_array32, array_size, 10, "\xdd\x00\x00\x00\x0a", 5);
  test_format(cmp_write_array, array_size, 10, "\x9a", 1);

  return true;
}

bool run_map_tests(void) {
  buf_t buf;
  cmp_ctx_t cmp;
  cmp_object_t obj;

  setup_cmp_and_buf(&cmp, &buf);

  test_format(cmp_write_fixmap, map_size, 10, "\x8a", 1);
  test_format(cmp_write_map16, map_size, 10, "\xde\x00\x0a", 3);
  test_format(cmp_write_map32, map_size, 10, "\xdf\x00\x00\x00\x0a", 5);
  test_format(cmp_write_map, map_size, 10, "\x8a", 1);

  return true;
}

bool run_ext_tests(void) {
  buf_t buf;
  cmp_ctx_t cmp;
  cmp_object_t obj;

  setup_cmp_and_buf(&cmp, &buf);

  test_fixext_format(cmp_write_fixext1, 1, 1, "C", "\xd4\x01\x43", 3);
  test_fixext_format(cmp_write_fixext2, 2, 2, "CC", "\xd5\x02\x43\x43", 4);
  test_fixext_format(
    cmp_write_fixext4, 3, 4, "CCCC", "\xd6\x03\x43\x43\x43\x43", 6
  );
  test_fixext_format(
    cmp_write_fixext8,
    4,
    8,
    "CCCCCCCC",
    "\xd7\x04\x43\x43\x43\x43\x43\x43\x43\x43",
    10
  );
  test_fixext_format(
    cmp_write_fixext16,
    5,
    16,
    "CCCCCCCCCCCCCCCC",
    "\xd8\x05\x43\x43\x43\x43\x43\x43\x43\x43\x43\x43\x43\x43\x43\x43\x43\x43",
    18
  );
  test_ext_format(cmp_write_ext8, 1, 1, "C", "\xc7\x01\x01\x43", 4);
  test_ext_format(
    cmp_write_ext8, 2, 3, "CCC", "\xc7\x02\x03\x43\x43\x43", 6
  );
  test_ext_format(cmp_write_ext16, 1, 1, "C", "\xc8\x01\x00\x01\x43", 5);
  test_ext_format(
    cmp_write_ext16, 2, 3, "CCC", "\xc8\x02\x00\x03\x43\x43\x43", 7
  );
  test_ext_format(
    cmp_write_ext32, 1, 1, "C", "\xc9\x01\x00\x00\x00\x01\x43", 7
  );
  test_ext_format(
    cmp_write_ext32, 2, 3, "CCC", "\xc9\x02\x00\x00\x00\x03\x43\x43\x43", 9
  );
  test_ext_format(cmp_write_ext, 1, 1, "C", "\xd4\x01\x43", 3);
  test_ext_format(
    cmp_write_ext, 2, 3, "CCC", "\xc7\x02\x03\x43\x43\x43", 6
  );

  return true;
}

int main(void) {
  printf("=== Testing CMP v%u (MessagePack v%u) ===\n\n",
    cmp_version(), cmp_mp_version()
  );

  M_BufferInitWithCapacity(&error_message, 4096);

  run_tests(msgpack);
  run_tests(fixedint);
  run_tests(number);
  run_tests(nil);
  run_tests(boolean);
  run_tests(binary);
  run_tests(string);
  run_tests(array);
  run_tests(map);
  run_tests(ext);

  printf("\nAll tests pass!\n\n");
  return EXIT_SUCCESS;
}

/* vi: set et ts=2 sw=2: */

