#define _CRT_SECURE_NO_WARNINGS   /* original_import_for_tests.h uses fopen */
#include "fake_backend.h"
#include "revenge/audio.h"
#include "Data/AudioData.h"
#include "Data/StartupData.h"

#include "original_import_for_tests.h"
#include <assert.h>

int main(void)
{
    static uint8_t informationSample[0x3442];
    RequireOriginalImport();

    FakeBackend_Reset();

    /* The assertions read the exact DS:5CFC catalog embedded in Original. */
    assert(FindDigitalSoundEntryByCode('I') == 0x00);
    assert(FindDigitalSoundEntryByCode('T') == 0x01);
    assert(FindDigitalSoundEntryByCode('=') == 0x02);
    assert(FindDigitalSoundEntryByCode('.') == 0x03);
    assert(FindDigitalSoundEntryByCode('J') == 0x0A);
    assert(FindDigitalSoundEntryByCode('r') == 0x15);
    assert(FindDigitalSoundEntryByCode('?') == -0x01);

    /* Scenario FFh is the one-code "I" sequence at DS:5E6E. Run it
	 * through the original descriptor builder to the speaker's speech,
	 * which plays the samples clean at 8000 Hz. */
    DigitalSoundBuffer = informationSample;
    NarrationMode = 0x01;
    SoundDriverIndex = 0x00;
    PlayScenarioDigitalSoundSequence(-0x01);
    assert(fakeBackend.buffer == informationSample);
    assert(fakeBackend.secondWord == sizeof(informationSample));
    assert(fakeBackend.firstWord == 0x1F40);
    assert(ActiveDigitalSampleDescriptor != NULL);
    assert(ActiveDigitalSampleDescriptor->SampleLength ==
        sizeof(informationSample));

    NarrationMode = 0x00;
    fakeBackend.buffer = NULL;
    PlayScenarioDigitalSoundSequence(-0x01);
    assert(fakeBackend.buffer == NULL);
    return 0x00;
}
