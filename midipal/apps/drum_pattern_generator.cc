// Copyright 2011 Olivier Gillet.
//
// Author: Olivier Gillet (ol.gillet@gmail.com)
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// -----------------------------------------------------------------------------
//
// Drum pattern generator app.

#include "midipal/apps/drum_pattern_generator.h"
#include "midipal/note_stack.h"

#include "midipal/clock.h"
#include "midipal/display.h"
#include "midipal/ui.h"

namespace midipal { namespace apps {
  
static const prog_uint8_t sizes[12] PROGMEM = {
  0, 1, 2, 3, 4, 5, 6, 8, 10, 12, 14, 16  
};

void DrumPatternGenerator::OnInit() {
  ui.AddPage(STR_RES_MOD, STR_RES_PTN, 0, 1);
  ui.AddPage(STR_RES_CLK, STR_RES_INT, 0, 1);
  ui.AddPage(STR_RES_BPM, UNIT_INTEGER, 40, 240);
  ui.AddPage(STR_RES_GRV, STR_RES_SWG, 0, 5);
  ui.AddPage(STR_RES_AMT, UNIT_INTEGER, 0, 127);
  ui.AddPage(STR_RES_CHN, UNIT_INDEX, 0, 15);
  for (uint8_t i = 0; i < kNumDrumParts; ++i) {
    ui.AddPage(STR_RES_PT1 + i, UNIT_NOTE, 20, 108);
  }
  clock.Update(bpm_, groove_template_, groove_amount_);
  clock.Start();
  
  memset(&active_pattern_[0], 0, kNumDrumParts);
  for (uint8_t i = 0; i < kNumDrumParts; ++i) {
    euclidian_num_notes_[i] = 0;
    euclidian_num_steps_[i] = 11; // 16 steps
  }
  idle_ticks_ = 96;
  running_ = 0;
}

void DrumPatternGenerator::Reset() {
  step_count_ = 0;
  bitmask_ = 1;
  tick_ = 0;
}

void DrumPatternGenerator::OnRawMidiData(
   uint8_t status,
   uint8_t* data,
   uint8_t data_size,
   uint8_t accepted_channel) {
  // Forward everything except note on for the selected channel.
  if (status != (0x80 | channel_) && 
      status != (0x90 | channel_)) {
    Send(status, data, data_size);
  }
}

void DrumPatternGenerator::OnContinue() {
  if (clk_mode_ == CLOCK_MODE_EXTERNAL) {
    running_ = 1;
  }
}

void DrumPatternGenerator::OnStart() {
  if (clk_mode_ == CLOCK_MODE_EXTERNAL) {
    running_ = 1;
    Reset();
  }
}

void DrumPatternGenerator::OnStop() {
  if (clk_mode_ == CLOCK_MODE_EXTERNAL) {
    running_ = 0;
  }
  AllNotesOff();
}

void DrumPatternGenerator::OnClock() {
  if (clk_mode_ == CLOCK_MODE_EXTERNAL && running_) {
    Tick();
  }
}

void DrumPatternGenerator::OnInternalClockTick() {
  if (clk_mode_ == CLOCK_MODE_INTERNAL && running_) {
    SendNow(0xf8);
    Tick();
  }
}

void DrumPatternGenerator::OnNoteOn(
    uint8_t channel,
    uint8_t note,
    uint8_t velocity) {
  if (channel != channel_) {
    return;
  }
  if (clk_mode_ == CLOCK_MODE_INTERNAL) {
    // Reset the clock counter.
    if (idle_ticks_ >= 96) {
      clock.Start();
      Reset();
      running_ = 1;
      SendNow(0xfa);
    }
    idle_ticks_ = 0;
  }
  note_stack.NoteOn(note, velocity);
  if (mode_ == 0) {
    // Select pattern depending on played notes.
    for (uint8_t i = 0; i < note_stack.size(); ++i) {
      Note n = FactorizeMidiNote(note_stack.sorted_note(i).note);
      uint8_t part = (n.octave >= 7) ? 3 : (n.octave <= 3 ? 0 : n.octave - 3);
      active_pattern_[part] = n.note;
    }
  } else {
    // Select pattern depending on played notes.
    uint8_t previous_octave = 0xff;
    for (uint8_t i = 0; i < note_stack.size(); ++i) {
      Note n = FactorizeMidiNote(note_stack.sorted_note(i).note);
      uint8_t part = (n.octave >= 7) ? 3 : (n.octave <= 3 ? 0 : n.octave - 3);
      if (n.octave == previous_octave) {
        euclidian_num_steps_[part] = n.note;
      } else {
        euclidian_num_notes_[part] = n.note;
        euclidian_step_count_[part] = 0;
        euclidian_bitmask_[part] = 1;
      }
      previous_octave = n.octave;
    }
  }
}

void DrumPatternGenerator::OnNoteOff(
    uint8_t channel,
    uint8_t note,
    uint8_t velocity) {
  if (channel != channel_) {
    return;
  }
  note_stack.NoteOff(note);
}

void DrumPatternGenerator::Tick() {
  if (!running_) {
    return;
  }
  ++tick_;
  
  // Stop the internal clock when no key has been pressed for the duration
  // of 1 bar.
  ++idle_ticks_;
  if (idle_ticks_ >= 96) {
    idle_ticks_ = 96;
    if (clk_mode_ == CLOCK_MODE_INTERNAL) {
      running_ = 0;
      AllNotesOff();
      SendNow(0xfc);
    }
  }
  if (tick_ == 6) {
    tick_ = 0;
    if (mode_ == 0) {
      // Preset pattern.
      uint8_t offset = 0;
      for (uint8_t part = 0; part < 4; ++part) {
        if (active_pattern_[part]) {
          idle_ticks_ = 0;
        }
        uint16_t pattern = ResourcesManager::Lookup<uint16_t, uint8_t>(
              lut_res_drum_patterns, active_pattern_[part] + offset);
        offset += 12;
        if (pattern & bitmask_) {
          TriggerNote(part);
        }
      }
      ++step_count_;
      bitmask_ <<= 1;
      if (step_count_ == 16) {
        step_count_ = 0;
        bitmask_ = 1;
      }
    } else {
      // Euclidian pattern.
      for (uint8_t part = 0; part < kNumDrumParts; ++part) {
        // Skip the muted parts.
        if (euclidian_num_notes_[part]) {
          // Continue running the sequencer as long as something is playing.
          idle_ticks_ = 0;
          uint16_t pattern = ResourcesManager::Lookup<uint16_t, uint8_t>(
              lut_res_euclidian_patterns,
              euclidian_num_notes_[part] + euclidian_num_steps_[part] * 12);
          uint16_t mask = euclidian_bitmask_[part];
          if (pattern & mask) {
            TriggerNote(part);
          }
        }
        ++euclidian_step_count_[part];
        euclidian_bitmask_[part] <<= 1;
        if (euclidian_step_count_[part] == ResourcesManager::Lookup<uint16_t, uint8_t>(
                  sizes, euclidian_num_steps_[part])) {
          euclidian_step_count_[part] = 0;
          euclidian_bitmask_[part] = 1;
        }
      }
    }
  } else if (tick_ == 3) {
    AllNotesOff();
  }
}

void DrumPatternGenerator::TriggerNote(uint8_t part) {
  Send3(0x90 | channel_, part_instrument_[part], 0x64);
  active_note_[part] = part_instrument_[part];
}

void DrumPatternGenerator::AllNotesOff() {
  for (uint8_t part = 0; part < kNumDrumParts; ++part) {
    if (active_note_[part]) {
      Send3(0x80 | channel_, part_instrument_[part], 0);
      active_note_[part] = 0;
    }
  }
}

void DrumPatternGenerator::SetParameter(uint8_t key, uint8_t value) {
  static_cast<uint8_t*>(&mode_)[key] = value;
  if (key < 4) {
    clock.Update(bpm_, groove_template_, groove_amount_);
  }
}

} }  // namespace midipal::apps
