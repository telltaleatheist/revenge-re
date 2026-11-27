#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "revenge/audio_log.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

/* The audio log's lines (revenge/audio_log.h): the sequencer's bytes cut
 * into messages (running status, system exclusive, real-time bytes inside a
 * message, a reset), the speaker's tone only when it changes, the samples
 * and the snaps, each with the clock's time in microseconds. */

static uint64_t now;

static uint64_t Clock(void)
{
    return now;
}

static void Bytes(const uint8_t *bytes, size_t count)
{
    size_t index;

    for (index = 0; index < count; ++index)
        AudioLog_MidiByte(bytes[index]);
}

int main(void)
{
    static const uint8_t notes[] = { 0x90, 0x3C, 0x64, 0x3E, 0x50, 0xC1, 0x05, 0xF8, 0xB0, 0x07, 0x7F };
    static const uint8_t exclusive[] = { 0xF0, 0x41, 0x10, 0xF7, 0x80, 0x3C, 0x00 };
    static const char *expected =
        "0 midi 90 3C 64\n"
        "0 midi 90 3E 50\n"
        "0 midi C1 05\n"
        "0 midi F8\n"
        "0 midi B0 07 7F\n"
        "1000 sysex 3 3A968160A35BA757\n"
        "1000 midi 80 3C 00\n"
        "2000 midi reset\n"
        "2000 midi -- 40\n"
        "3000 tone 1234\n"
        "4000 tone 0\n"
        "5000 tone-effect 7\n"
        "5000 pcm effect 2 7920\n"
        "6000 pcm done\n"
        "6000 snap end\n";
    const char *path = "audio-log-test.txt";
    char text[1024];
    size_t length;
    FILE *file;

    AudioLog_MidiByte(0x90);
    AudioLog_Snap("unopened");
    AudioLog_Open(path, Clock);
    Bytes(notes, sizeof(notes));
    now = 1000000;
    Bytes(exclusive, sizeof(exclusive));
    now = 2000000;
    AudioLog_MidiReset();
    AudioLog_MidiByte(0x40);
    now = 3000000;
    AudioLog_SpeakerDivisor(1234);
    AudioLog_SpeakerGate(1);
    AudioLog_SpeakerGate(1);
    now = 4000000;
    AudioLog_SpeakerGate(0);
    AudioLog_SpeakerDivisor(99);
    now = 5000000;
    AudioLog_ToneEffect(7);
    AudioLog_PcmStart("effect", 2, 7920);
    now = 6000000;
    AudioLog_PcmDone();
    AudioLog_Snap("end");
    AudioLog_Close();

    file = fopen(path, "rb");
    assert(file != NULL);
    length = fread(text, 1, sizeof(text) - 1, file);
    fclose(file);
    text[length] = '\0';
    if (strcmp(text, expected) != 0)
    {
        fprintf(stderr, "audio log:\n%s\nexpected:\n%s", text, expected);
        return 1;
    }
    (void) remove(path);
    return 0;
}
