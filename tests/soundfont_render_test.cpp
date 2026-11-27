// The soundfont subset plays as the full bank. From the MW1 rebuild (mech1-re),
// tests/soundfont_render_test.cpp, unchanged but for this line.
//
//     soundfont_render_test FULL.sf2 SUBSET.sf2 BANK:PROGRAM...
//
// For each listed preset, through TinySoundFont's own loading:
// - exactly: the same regions, field for field (sample positions taken from
//   the region's own start), and the same sample points under every region.
//   These are all the synthesizer's inputs.
// - rendered: every 3rd key from 12 to 108 at two velocities, held then
//   released, then a chord. The output must agree within one 16-bit step.
//   It can't be bit-identical: TinySoundFont keeps a voice's position as an
//   absolute double index into the whole bank, so the same note interpolates
//   with slightly different rounding at a different offset (about -100 dB).
// The subset must also hold nothing else. Prints counts only; run by
// tools/soundfont/subset.py check and test_subset.py.
#define TSF_IMPLEMENTATION
#include "tsf.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

namespace {

constexpr int kRate = 22050;
constexpr int kHold = kRate / 4;     // 250 ms held
constexpr int kRelease = kRate / 2;  // 500 ms of release
constexpr float kOneStep = 1.0f / 32768.0f;

bool SameRegion(const tsf* fa, const tsf_region& a, const tsf* fb, const tsf_region& b) {
  auto rel = [](const tsf_region& r, unsigned v) { return v - r.offset; };
  const bool fields =
      a.loop_mode == b.loop_mode && a.sample_rate == b.sample_rate && a.lokey == b.lokey && a.hikey == b.hikey &&
      a.lovel == b.lovel && a.hivel == b.hivel && a.group == b.group && rel(a, a.end) == rel(b, b.end) &&
      rel(a, a.loop_start) == rel(b, b.loop_start) && rel(a, a.loop_end) == rel(b, b.loop_end) &&
      a.transpose == b.transpose && a.tune == b.tune && a.pitch_keycenter == b.pitch_keycenter &&
      a.pitch_keytrack == b.pitch_keytrack && a.attenuation == b.attenuation && a.pan == b.pan &&
      std::memcmp(&a.ampenv, &b.ampenv, sizeof a.ampenv) == 0 && std::memcmp(&a.modenv, &b.modenv, sizeof a.modenv) == 0 &&
      a.initialFilterQ == b.initialFilterQ && a.initialFilterFc == b.initialFilterFc &&
      a.modEnvToPitch == b.modEnvToPitch && a.modEnvToFilterFc == b.modEnvToFilterFc &&
      a.modLfoToFilterFc == b.modLfoToFilterFc && a.modLfoToVolume == b.modLfoToVolume &&
      a.delayModLFO == b.delayModLFO && a.freqModLFO == b.freqModLFO && a.modLfoToPitch == b.modLfoToPitch &&
      a.delayVibLFO == b.delayVibLFO && a.freqVibLFO == b.freqVibLFO && a.vibLfoToPitch == b.vibLfoToPitch;
  if (!fields) return false;
  // Every point the voice can read: from the start to the end or the loop's end, whichever is later
  // (TinySoundFont's loader has already kept both within its bank).
  const unsigned span = std::max(rel(a, a.end), rel(a, a.loop_end));
  return std::memcmp(fa->fontSamples + a.offset, fb->fontSamples + b.offset, span * sizeof(float)) == 0;
}

std::vector<float> Render(tsf* bank, int presetIndex) {
  std::vector<float> out, block;
  auto run = [&](int frames) {
    block.assign(static_cast<size_t>(frames) * 2, 0.0f);
    tsf_render_float(bank, block.data(), frames, 0);
    out.insert(out.end(), block.begin(), block.end());
  };
  for (float velocity : {0.4f, 1.0f}) {
    for (int key = 12; key <= 108; key += 3) {
      tsf_note_on(bank, presetIndex, key, velocity);
      run(kHold);
      tsf_note_off(bank, presetIndex, key);
      run(kRelease);
    }
  }
  for (int key : {48, 55, 60, 64}) tsf_note_on(bank, presetIndex, key, 0.8f);
  run(kHold);
  tsf_note_off_all(bank);
  run(kRelease);
  tsf_reset(bank);
  return out;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 4) {
    std::fprintf(stderr, "usage: %s FULL.sf2 SUBSET.sf2 BANK:PROGRAM...\n", argv[0]);
    return 2;
  }
  tsf* full = tsf_load_filename(argv[1]);
  tsf* part = tsf_load_filename(argv[2]);
  if (!full || !part) {
    std::printf("FAIL a bank did not load\n");
    return 1;
  }
  for (tsf* bank : {full, part}) tsf_set_output(bank, TSF_STEREO_INTERLEAVED, kRate, 0.0f);
  int failures = 0, regions = 0;
  float worstAll = 0.0f;
  for (int i = 3; i < argc; ++i) {
    int bankNumber = 0, program = 0;
    if (std::sscanf(argv[i], "%d:%d", &bankNumber, &program) != 2) {
      std::printf("FAIL bad preset %s\n", argv[i]);
      return 2;
    }
    const int a = tsf_get_presetindex(full, bankNumber, program);
    const int b = tsf_get_presetindex(part, bankNumber, program);
    if (a < 0 || b < 0) {
      std::printf("FAIL %d:%d is missing from the %s\n", bankNumber, program, a < 0 ? "full bank" : "subset");
      ++failures;
      continue;
    }
    const tsf_preset &pa = full->presets[a], &pb = part->presets[b];
    bool same = pa.regionNum == pb.regionNum && pa.regionNum > 0;
    for (int r = 0; same && r < pa.regionNum; ++r) same = SameRegion(full, pa.regions[r], part, pb.regions[r]);
    regions += pa.regionNum;
    if (!same) {
      std::printf("FAIL %d:%d: its regions or samples differ in the subset\n", bankNumber, program);
      ++failures;
      continue;
    }
    const std::vector<float> x = Render(full, a), y = Render(part, b);
    float worst = x.size() == y.size() ? 0.0f : 1.0f, peak = 0.0f;
    for (size_t k = 0; k < x.size() && k < y.size(); ++k) {
      worst = std::max(worst, std::fabs(x[k] - y[k]));
      peak = std::max(peak, std::fabs(x[k]));
    }
    worstAll = std::max(worstAll, worst);
    if (peak == 0.0f || worst >= kOneStep) {
      std::printf("FAIL %d:%d renders %s\n", bankNumber, program, peak == 0.0f ? "silence" : "differently");
      ++failures;
    }
  }
  if (tsf_get_presetcount(part) != argc - 3) {
    std::printf("FAIL the subset holds %d presets, %d listed\n", tsf_get_presetcount(part), argc - 3);
    ++failures;
  }
  tsf_close(full);
  tsf_close(part);
  std::printf("%d presets, %d regions: %s; render within %.1f dB of a 16-bit step\n", argc - 3, regions,
              failures ? "FAILED" : "identical inputs", worstAll > 0 ? 20.0 * std::log10(worstAll / kOneStep) : -999.0);
  return failures ? 1 : 0;
}
