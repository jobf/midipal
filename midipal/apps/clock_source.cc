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
// MIDI clock generator app.

#include "midipal/apps/clock_source.h"

#include "avrlib/string.h"

#include "midipal/clock.h"
#include "midipal/display.h"
#include "midipal/ui.h"

namespace midipal { namespace apps {

const prog_uint8_t clock_source_factory_data[4] PROGMEM = {
  0, 120, 0, 0
};

/* static */
uint8_t ClockSource::running_;

/* static */
uint8_t ClockSource::bpm_;

/* static */
uint8_t ClockSource::groove_template_;

/* static */
uint8_t ClockSource::groove_amount_;

/* static */
const prog_AppInfo ClockSource::app_info_ PROGMEM = {
  &OnInit, // void (*OnInit)();
  NULL, // void (*OnNoteOn)(uint8_t, uint8_t, uint8_t);
  NULL, // void (*OnNoteOff)(uint8_t, uint8_t, uint8_t);
  NULL, // void (*OnNoteAftertouch)(uint8_t, uint8_t, uint8_t);
  NULL, // void (*OnAftertouch)(uint8_t, uint8_t);
  NULL, // void (*OnControlChange)(uint8_t, uint8_t, uint8_t);
  NULL, // void (*OnProgramChange)(uint8_t, uint8_t);
  NULL, // void (*OnPitchBend)(uint8_t, uint16_t);
  NULL, // void (*OnAllSoundOff)(uint8_t);
  NULL, // void (*OnResetAllControllers)(uint8_t);
  NULL, // void (*OnLocalControl)(uint8_t, uint8_t);
  NULL, // void (*OnAllNotesOff)(uint8_t);
  NULL, // void (*OnOmniModeOff)(uint8_t);
  NULL, // void (*OnOmniModeOn)(uint8_t);
  NULL, // void (*OnMonoModeOn)(uint8_t, uint8_t);
  NULL, // void (*OnPolyModeOn)(uint8_t);
  NULL, // void (*OnSysExStart)();
  NULL, // void (*OnSysExByte)(uint8_t);
  NULL, // void (*OnSysExEnd)();
  NULL, // void (*OnClock)();
  &OnStart, // void (*OnStart)();
  &OnContinue, // void (*OnContinue)();
  &OnStop, // void (*OnStop)();
  NULL, // void (*OnActiveSensing)();
  NULL, // void (*OnReset)();
  NULL, // uint8_t (*CheckChannel)(uint8_t);
  &OnRawByte, // void (*OnRawByte)(uint8_t);
  NULL, // void (*OnRawMidiData)(uint8_t, uint8_t*, uint8_t, uint8_t);
  &OnInternalClockTick, // void (*OnInternalClockTick)();
  NULL, // void (*OnInternalClockStep)();
  NULL, // uint8_t (*OnIncrement)(int8_t);
  NULL, // uint8_t (*OnClick)();
  NULL, // uint8_t (*OnPot)(uint8_t, uint8_t);
  NULL, // uint8_t (*OnRedraw)();
  NULL, // void (*OnIdle)();
  &SetParameter, // void (*SetParameter)(uint8_t, uint8_t);
  NULL, // uint8_t (*GetParameter)(uint8_t);
  NULL, // uint8_t (*CheckPageStatus)(uint8_t);
  4, // settings_size
  SETTINGS_CLOCK_SOURCE, // settings_offset
  &running_, // settings_data
  clock_source_factory_data, // factory_data
  STR_RES_CLOCK, // app_name
};

/* static */
void ClockSource::OnInit() {
  ui.AddPage(STR_RES_RUN, STR_RES_OFF, 0, 1);
  ui.AddPage(STR_RES_BPM, UNIT_INTEGER, 40, 240);
  ui.AddPage(STR_RES_GRV, STR_RES_SWG, 0, 5);
  ui.AddPage(STR_RES_AMT, UNIT_INTEGER, 0, 127);
  clock.Update(bpm_, groove_template_, groove_amount_);
  running_ = 0;
}

/* static */
void ClockSource::OnRawByte(uint8_t byte) {
  uint8_t is_realtime = (byte & 0xf0) == 0xf0;
  uint8_t is_sysex = (byte == 0xf7) || (byte == 0xf0);
  if (!is_realtime || is_sysex) {
    app.SendNow(byte);
  }
}

/* static */
void ClockSource::SetParameter(uint8_t key, uint8_t value) {
  if (key == 0) {
    if (value == 1) {
      Start();
    } else {
      Stop();
    }
  }
  static_cast<uint8_t*>(&running_)[key] = value;
  clock.Update(bpm_, groove_template_, groove_amount_);
}

/* static */
void ClockSource::OnStart() {
  Start();
}

/* static */
void ClockSource::OnStop() {
  Stop();
}

/* static */
void ClockSource::OnContinue() {
  Start();
}

/* static */
void ClockSource::OnInternalClockTick() {
  app.SendNow(0xf8);
}

/* static */
void ClockSource::Stop() {
  if (running_) {
    clock.Stop();
    app.SendNow(0xfc);
    running_ = 0;
  }
}

/* static */
void ClockSource::Start() {
  if (!running_) {
    clock.Start();
    app.SendNow(0xfa);
    running_ = 1;
  }
}

} }  // namespace midipal::apps
