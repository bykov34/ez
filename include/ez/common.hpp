
#pragma once

#define ROL8(a, n) (((a) << (n)) | ((a) >> (8 - (n))))
#define ROL16(a, n) (((a) << (n)) | ((a) >> (16 - (n))))
#define ROL32(a, n) (((a) << (n)) | ((a) >> (32 - (n))))
#define ROL64(a, n) (((a) << (n)) | ((a) >> (64 - (n))))

#define ROR8(a, n) (((a) >> (n)) | ((a) << (8 - (n))))
#define ROR16(a, n) (((a) >> (n)) | ((a) << (16 - (n))))
#define ROR32(a, n) (((a) >> (n)) | ((a) << (32 - (n))))
#define ROR64(a, n) (((a) >> (n)) | ((a) << (64 - (n))))

#define SHL8(a, n) ((a) << (n))
#define SHL16(a, n) ((a) << (n))
#define SHL32(a, n) ((a) << (n))
#define SHL64(a, n) ((a) << (n))

#define SHR8(a, n) ((a) >> (n))
#define SHR16(a, n) ((a) >> (n))
#define SHR32(a, n) ((a) >> (n))
#define SHR64(a, n) ((a) >> (n))

//Load unaligned 16-bit integer (little-endian encoding)
#define LOAD16LE(p) ( \
   ((uint16_t)(((uint8_t *)(p))[0]) << 0) | \
   ((uint16_t)(((uint8_t *)(p))[1]) << 8))

//Load unaligned 16-bit integer (big-endian encoding)
#define LOAD16BE(p) ( \
   ((uint16_t)(((uint8_t *)(p))[0]) << 8) | \
   ((uint16_t)(((uint8_t *)(p))[1]) << 0))

//Load unaligned 24-bit integer (little-endian encoding)
#define LOAD24LE(p) ( \
   ((uint32_t)(((uint8_t *)(p))[0]) << 0)| \
   ((uint32_t)(((uint8_t *)(p))[1]) << 8) | \
   ((uint32_t)(((uint8_t *)(p))[2]) << 16))

//Load unaligned 24-bit integer (big-endian encoding)
#define LOAD24BE(p) ( \
   ((uint32_t)(((uint8_t *)(p))[0]) << 16) | \
   ((uint32_t)(((uint8_t *)(p))[1]) << 8) | \
   ((uint32_t)(((uint8_t *)(p))[2]) << 0))

//Load unaligned 32-bit integer (little-endian encoding)
#define LOAD32LE(p) ( \
   ((uint32_t)(((uint8_t *)(p))[0]) << 0) | \
   ((uint32_t)(((uint8_t *)(p))[1]) << 8) | \
   ((uint32_t)(((uint8_t *)(p))[2]) << 16) | \
   ((uint32_t)(((uint8_t *)(p))[3]) << 24))

//Load unaligned 32-bit integer (big-endian encoding)
#define LOAD32BE(p) ( \
   ((uint32_t)(((uint8_t *)(p))[0]) << 24) | \
   ((uint32_t)(((uint8_t *)(p))[1]) << 16) | \
   ((uint32_t)(((uint8_t *)(p))[2]) << 8) | \
   ((uint32_t)(((uint8_t *)(p))[3]) << 0))

//Load unaligned 48-bit integer (little-endian encoding)
#define LOAD48LE(p) ( \
   ((uint64_t)(((uint8_t *)(p))[0]) << 0) | \
   ((uint64_t)(((uint8_t *)(p))[1]) << 8) | \
   ((uint64_t)(((uint8_t *)(p))[2]) << 16) | \
   ((uint64_t)(((uint8_t *)(p))[3]) << 24) | \
   ((uint64_t)(((uint8_t *)(p))[4]) << 32) | \
   ((uint64_t)(((uint8_t *)(p))[5]) << 40)

//Load unaligned 48-bit integer (big-endian encoding)
#define LOAD48BE(p) ( \
   ((uint64_t)(((uint8_t *)(p))[0]) << 40) | \
   ((uint64_t)(((uint8_t *)(p))[1]) << 32) | \
   ((uint64_t)(((uint8_t *)(p))[2]) << 24) | \
   ((uint64_t)(((uint8_t *)(p))[3]) << 16) | \
   ((uint64_t)(((uint8_t *)(p))[4]) << 8) | \
   ((uint64_t)(((uint8_t *)(p))[5]) << 0))

//Load unaligned 64-bit integer (little-endian encoding)
#define LOAD64LE(p) ( \
   ((uint64_t)(((uint8_t *)(p))[0]) << 0) | \
   ((uint64_t)(((uint8_t *)(p))[1]) << 8) | \
   ((uint64_t)(((uint8_t *)(p))[2]) << 16) | \
   ((uint64_t)(((uint8_t *)(p))[3]) << 24) | \
   ((uint64_t)(((uint8_t *)(p))[4]) << 32) | \
   ((uint64_t)(((uint8_t *)(p))[5]) << 40) | \
   ((uint64_t)(((uint8_t *)(p))[6]) << 48) | \
   ((uint64_t)(((uint8_t *)(p))[7]) << 56))

//Load unaligned 64-bit integer (big-endian encoding)
#define LOAD64BE(p) ( \
   ((uint64_t)(((uint8_t *)(p))[0]) << 56) | \
   ((uint64_t)(((uint8_t *)(p))[1]) << 48) | \
   ((uint64_t)(((uint8_t *)(p))[2]) << 40) | \
   ((uint64_t)(((uint8_t *)(p))[3]) << 32) | \
   ((uint64_t)(((uint8_t *)(p))[4]) << 24) | \
   ((uint64_t)(((uint8_t *)(p))[5]) << 16) | \
   ((uint64_t)(((uint8_t *)(p))[6]) << 8) | \
   ((uint64_t)(((uint8_t *)(p))[7]) << 0))

//Store unaligned 16-bit integer (little-endian encoding)
#define STORE16LE(a, p) \
   ((uint8_t *)(p))[0] = ((uint16_t)(a) >> 0) & 0xFFU, \
   ((uint8_t *)(p))[1] = ((uint16_t)(a) >> 8) & 0xFFU

//Store unaligned 32-bit integer (big-endian encoding)
#define STORE16BE(a, p) \
   ((uint8_t *)(p))[0] = ((uint16_t)(a) >> 8) & 0xFFU, \
   ((uint8_t *)(p))[1] = ((uint16_t)(a) >> 0) & 0xFFU

//Store unaligned 24-bit integer (little-endian encoding)
#define STORE24LE(a, p) \
   ((uint8_t *)(p))[0] = ((uint32_t)(a) >> 0) & 0xFFU, \
   ((uint8_t *)(p))[1] = ((uint32_t)(a) >> 8) & 0xFFU, \
   ((uint8_t *)(p))[2] = ((uint32_t)(a) >> 16) & 0xFFU

//Store unaligned 24-bit integer (big-endian encoding)
#define STORE24BE(a, p) \
   ((uint8_t *)(p))[0] = ((uint32_t)(a) >> 16) & 0xFFU, \
   ((uint8_t *)(p))[1] = ((uint32_t)(a) >> 8) & 0xFFU, \
   ((uint8_t *)(p))[2] = ((uint32_t)(a) >> 0) & 0xFFU

//Store unaligned 32-bit integer (little-endian encoding)
#define STORE32LE(a, p) \
   ((uint8_t *)(p))[0] = ((uint32_t)(a) >> 0) & 0xFFU, \
   ((uint8_t *)(p))[1] = ((uint32_t)(a) >> 8) & 0xFFU, \
   ((uint8_t *)(p))[2] = ((uint32_t)(a) >> 16) & 0xFFU, \
   ((uint8_t *)(p))[3] = ((uint32_t)(a) >> 24) & 0xFFU

//Store unaligned 32-bit integer (big-endian encoding)
#define STORE32BE(a, p) \
   ((uint8_t *)(p))[0] = ((uint32_t)(a) >> 24) & 0xFFU, \
   ((uint8_t *)(p))[1] = ((uint32_t)(a) >> 16) & 0xFFU, \
   ((uint8_t *)(p))[2] = ((uint32_t)(a) >> 8) & 0xFFU, \
   ((uint8_t *)(p))[3] = ((uint32_t)(a) >> 0) & 0xFFU

//Store unaligned 48-bit integer (little-endian encoding)
#define STORE48LE(a, p) \
   ((uint8_t *)(p))[0] = ((uint64_t)(a) >> 0) & 0xFFU, \
   ((uint8_t *)(p))[1] = ((uint64_t)(a) >> 8) & 0xFFU, \
   ((uint8_t *)(p))[2] = ((uint64_t)(a) >> 16) & 0xFFU, \
   ((uint8_t *)(p))[3] = ((uint64_t)(a) >> 24) & 0xFFU, \
   ((uint8_t *)(p))[4] = ((uint64_t)(a) >> 32) & 0xFFU, \
   ((uint8_t *)(p))[5] = ((uint64_t)(a) >> 40) & 0xFFU,

//Store unaligned 48-bit integer (big-endian encoding)
#define STORE48BE(a, p) \
   ((uint8_t *)(p))[0] = ((uint64_t)(a) >> 40) & 0xFFU, \
   ((uint8_t *)(p))[1] = ((uint64_t)(a) >> 32) & 0xFFU, \
   ((uint8_t *)(p))[2] = ((uint64_t)(a) >> 24) & 0xFFU, \
   ((uint8_t *)(p))[3] = ((uint64_t)(a) >> 16) & 0xFFU, \
   ((uint8_t *)(p))[4] = ((uint64_t)(a) >> 8) & 0xFFU, \
   ((uint8_t *)(p))[5] = ((uint64_t)(a) >> 0) & 0xFFU

//Store unaligned 64-bit integer (little-endian encoding)
#define STORE64LE(a, p) \
   ((uint8_t *)(p))[0] = ((uint64_t)(a) >> 0) & 0xFFU, \
   ((uint8_t *)(p))[1] = ((uint64_t)(a) >> 8) & 0xFFU, \
   ((uint8_t *)(p))[2] = ((uint64_t)(a) >> 16) & 0xFFU, \
   ((uint8_t *)(p))[3] = ((uint64_t)(a) >> 24) & 0xFFU, \
   ((uint8_t *)(p))[4] = ((uint64_t)(a) >> 32) & 0xFFU, \
   ((uint8_t *)(p))[5] = ((uint64_t)(a) >> 40) & 0xFFU, \
   ((uint8_t *)(p))[6] = ((uint64_t)(a) >> 48) & 0xFFU, \
   ((uint8_t *)(p))[7] = ((uint64_t)(a) >> 56) & 0xFFU

//Store unaligned 64-bit integer (big-endian encoding)
#define STORE64BE(a, p) \
   ((uint8_t *)(p))[0] = ((uint64_t)(a) >> 56) & 0xFFU, \
   ((uint8_t *)(p))[1] = ((uint64_t)(a) >> 48) & 0xFFU, \
   ((uint8_t *)(p))[2] = ((uint64_t)(a) >> 40) & 0xFFU, \
   ((uint8_t *)(p))[3] = ((uint64_t)(a) >> 32) & 0xFFU, \
   ((uint8_t *)(p))[4] = ((uint64_t)(a) >> 24) & 0xFFU, \
   ((uint8_t *)(p))[5] = ((uint64_t)(a) >> 16) & 0xFFU, \
   ((uint8_t *)(p))[6] = ((uint64_t)(a) >> 8) & 0xFFU, \
   ((uint8_t *)(p))[7] = ((uint64_t)(a) >> 0) & 0xFFU

//Swap a 16-bit integer
#define SWAPINT16(x) ( \
   (((uint16_t)(x) & 0x00FFU) << 8) | \
   (((uint16_t)(x) & 0xFF00U) >> 8))

//Swap a 32-bit integer
#define SWAPINT32(x) ( \
   (((uint32_t)(x) & 0x000000FFUL) << 24) | \
   (((uint32_t)(x) & 0x0000FF00UL) << 8) | \
   (((uint32_t)(x) & 0x00FF0000UL) >> 8) | \
   (((uint32_t)(x) & 0xFF000000UL) >> 24))

//Swap a 64-bit integer
#define SWAPINT64(x) ( \
   (((uint64_t)(x) & 0x00000000000000FFULL) << 56) | \
   (((uint64_t)(x) & 0x000000000000FF00ULL) << 40) | \
   (((uint64_t)(x) & 0x0000000000FF0000ULL) << 24) | \
   (((uint64_t)(x) & 0x00000000FF000000ULL) << 8) | \
   (((uint64_t)(x) & 0x000000FF00000000ULL) >> 8) | \
   (((uint64_t)(x) & 0x0000FF0000000000ULL) >> 24) | \
   (((uint64_t)(x) & 0x00FF000000000000ULL) >> 40) | \
   (((uint64_t)(x) & 0xFF00000000000000ULL) >> 56))

