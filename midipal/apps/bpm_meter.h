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
// MIDI clock monitor app.

#ifndef MIDIPAL_APPS_BPM_METER_H_
#define MIDIPAL_APPS_BPM_METER_H_

#include "midipal/app.h"

namespace midipal { namespace apps {

class BpmMeter : public App {
 public:
  BpmMeter() { }

  void OnInit();

  void OnClock();
  void OnStart();
  void OnStop();
  void OnContinue();
  void OnReset();
  void OnRawByte(uint8_t byte);

  uint8_t OnClick();
  uint8_t OnIncrement(int8_t increment);
  uint8_t OnRedraw();
  
  uint8_t app_name() { return STR_RES_BPM_CNTR; }
  
 private:
  void PrintBpm();
  void Reset();

  uint32_t num_ticks_;
  uint32_t clock_;
  uint8_t active_page_;
  uint8_t refresh_bpm_;
  
  DISALLOW_COPY_AND_ASSIGN(BpmMeter);
};

} }  // namespace midipal::apps

#endif // MIDIPAL_APPS_MONITOR_H_