#ifndef REVENGE_AUDIO_LOG_H
#define REVENGE_AUDIO_LOG_H

/* The audio log (tools/trace/audio): what the game asks to be heard, line by
 * line, with the host clock's time in microseconds. It is written only when
 * the host opens it (REVENGE_AUDIO_LOG names the file); otherwise every call
 * returns at once. The lines are the game's requests, taken where the game
 * makes them, so they stay the same while the sound underneath them is
 * rebuilt:
 *
 *   T midi SS [D1 [D2]]   a channel or system message as the music sequencer
 *                         sends it to the music driver (hex bytes)
 *   T midi reset          the sequencer resets the music driver
 *   T sysex N H           a system-exclusive message of N bytes after F0, H
 *                         the first 16 hex digits of their SHA-256
 *   T tone D              the speaker's tone: the PIT divisor while the
 *                         speaker sounds, 0 when it is silent
 *   T tone-effect I       a speaker effect stream starts: effect I (its
 *                         tones follow as tone lines)
 *   T pcm effect I N      a digital effect starts: effect I, N bytes
 *   T pcm speech I N      a speech sample starts: entry I, N bytes
 *   T pcm done            the sample playing has ended
 *   T snap NAME           the harness took a snap
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Opens the log at path with now() as its clock; no path, no log. */
void AudioLog_Open(const char *path, uint64_t (*nowNanoseconds)(void));
void AudioLog_Close(void);

void AudioLog_MidiByte(uint8_t value);
void AudioLog_MidiReset(void);
void AudioLog_SpeakerGate(int sounding);
void AudioLog_SpeakerDivisor(uint16_t divisor);
void AudioLog_ToneEffect(unsigned index);
void AudioLog_PcmStart(const char *kind, unsigned index, unsigned length);
void AudioLog_PcmDone(void);
void AudioLog_Snap(const char *name);

#ifdef __cplusplus
}
#endif

#endif
