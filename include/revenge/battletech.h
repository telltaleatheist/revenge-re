#ifndef REVENGE_BATTLETECH_H
#define REVENGE_BATTLETECH_H

/* BattleTech rule and force geometry constants recovered from the original
 * executable. Keep these names at call sites so game rules remain distinct
 * from display dimensions and packed-file sizes that happen to share values. */
inline constexpr int BATTLETECH_D6_FACE_COUNT = 0x06;
inline constexpr int BATTLETECH_D6_CANDIDATE_MASK = 0x07;
inline constexpr int BATTLETECH_2D6_MINIMUM = 0x02;
inline constexpr int BATTLETECH_2D6_CRITICAL_TARGET = 0x08;
inline constexpr int BATTLETECH_MAX_TO_HIT_TARGET = 0x0A;
inline constexpr int BATTLETECH_2D6_NATURAL_TWELVE = 0x0C;
inline constexpr int BATTLETECH_CATASTROPHIC_LOCATION_DAMAGE = 0x64;
inline constexpr int BATTLETECH_DIRECTION_COUNT = 0x08;
inline constexpr int LANCE_UNIT_COUNT = 0x04;
inline constexpr int TACTICAL_FORCE_UNIT_COUNT = 0x0C;
inline constexpr int TACTICAL_FORCE_LAST_UNIT_OFFSET = TACTICAL_FORCE_UNIT_COUNT - 0x01;
inline constexpr int TACTICAL_UNIT_COUNT = TACTICAL_FORCE_UNIT_COUNT * 0x02;
inline constexpr int FRIENDLY_FORCE_FIRST_UNIT = 0x00;
inline constexpr int ENEMY_FORCE_FIRST_UNIT = TACTICAL_FORCE_UNIT_COUNT;

#endif
