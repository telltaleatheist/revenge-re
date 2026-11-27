#ifndef REVENGE_TYPES_H
#define REVENGE_TYPES_H

#include <assert.h> /* static_assert, in C17 */
#include <stdint.h>

typedef uint8_t byte;
typedef int8_t int8;
typedef int16_t int16;
typedef uint16_t uint16;
typedef uint16_t word16;
typedef uint32_t uint32;
typedef int32_t int32;

/*
 * The one language extension the rebuild allows (docs/CODE_STYLE.md): a
 * struct without a name inside a union, so a record's fields and its raw
 * bytes (or another view of the same bytes) are read by the same names as
 * the original read them. C11 has it; C++ compilers accept it as an
 * extension and warn. Only the three headers that hold such views
 * (revenge/unit.h, original/campaign_data.h, original/scene_data.h) open
 * this.
 */
#if defined(_MSC_VER)
#define REVENGE_ANONYMOUS_STRUCTS_BEGIN \
    __pragma(warning(push)) __pragma(warning(disable : 4201))
#define REVENGE_ANONYMOUS_STRUCTS_END __pragma(warning(pop))
#elif defined(__clang__)
#define REVENGE_ANONYMOUS_STRUCTS_BEGIN \
    _Pragma("clang diagnostic push") \
        _Pragma("clang diagnostic ignored \"-Wgnu-anonymous-struct\"") \
            _Pragma("clang diagnostic ignored \"-Wnested-anon-types\"")
#define REVENGE_ANONYMOUS_STRUCTS_END _Pragma("clang diagnostic pop")
#elif defined(__GNUC__)
#define REVENGE_ANONYMOUS_STRUCTS_BEGIN \
    _Pragma("GCC diagnostic push") \
        _Pragma("GCC diagnostic ignored \"-Wpedantic\"")
#define REVENGE_ANONYMOUS_STRUCTS_END _Pragma("GCC diagnostic pop")
#else
#define REVENGE_ANONYMOUS_STRUCTS_BEGIN
#define REVENGE_ANONYMOUS_STRUCTS_END
#endif

/*
 * Original one-byte identifiers use FFh as the unassigned sentinel.  Keep
 * that signed interpretation in their C types so callers do not have to
 * reproduce the DOS compiler's CBW instruction at every use.
 */
typedef int8_t RevengePilotId;
typedef int8_t RevengeUnitTypeId;
typedef int8_t RevengeUnitSlot;
typedef int8_t RevengeAssignmentSlot;

enum
{
    REVENGE_PILOT_ID_NONE = -1,
    REVENGE_UNIT_TYPE_NONE = -1,
    REVENGE_UNIT_SLOT_NONE = -1,
    REVENGE_UNIT_SLOT_UNAVAILABLE = -2,
    REVENGE_ASSIGNMENT_SLOT_NONE = -1
};

#pragma pack(push, 1)
/*
 * Exact on-disk/in-memory representation of a DOS offset:segment address.
 * This is binary data, not a host C pointer. Runtime objects use native
 * pointers and translate only when crossing an original DOS data boundary.
 */
typedef struct DosAddress
{
    uint16_t offset;
    uint16_t segment;
} DosAddress;
#pragma pack(pop)

static_assert(sizeof(DosAddress) == 0x04,
    "A DOS offset:segment address occupies four bytes.");

static inline DosAddress DosAddress_FromValue(uint32_t value)
{
    DosAddress pointer;

    pointer.offset = (uint16_t) value;
    pointer.segment = (uint16_t) (value >> 0x10);
    return pointer;
}

static inline uint32_t DosAddress_ToValue(DosAddress pointer)
{
    return ((uint32_t) pointer.segment << 0x10) | pointer.offset;
}

static inline int DosAddress_IsNull(DosAddress pointer)
{
    return pointer.offset == 0x0000 && pointer.segment == 0x0000;
}

inline constexpr int FALSE = 0x00;
inline constexpr int TRUE = 0x01;

#endif
