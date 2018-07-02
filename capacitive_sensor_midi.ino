//  Copyright 2015 Stanislav Senotrusov <stan@senotrusov.com>
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.

// Many thanks to Sasha Buttsynov (http://facebook.com/sasha.buttsynov) for helping me with the music theory.

// http://playground.arduino.cc/Main/CapacitiveSensor
// MIT License
#include <CapacitiveSensor.h>

// https://github.com/FortySevenEffects/arduino_midi_library
// MIT License
#include <MIDI.h>

#define DEBUG_SENSORS 0
#define CYCLE_OCTAVES 0

const int sensors_timeout = 20;
const int sensors_samples = 30;
const int sensors_alarm_threshold = 250;

CapacitiveSensor sensors[] = {
  CapacitiveSensor(12, 13),
  CapacitiveSensor(10, 11),
  CapacitiveSensor(8, 9),
  CapacitiveSensor(6, 7),
  CapacitiveSensor(4, 5),
  CapacitiveSensor(2, 3),
  CapacitiveSensor(A4, A5),
  CapacitiveSensor(A2, A3),
  CapacitiveSensor(A0, A1)
};

const int sensors_amount = sizeof(sensors) / sizeof(CapacitiveSensor);
unsigned long alarmed_sensors[sensors_amount];

/* MIDI NOTE NUMBERS
//   C  C#   D  D#   E   F  F#   G  G#   A  A#   B
     0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, //  0  C-2
    12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, //  1  C-1
    24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, //  2  C0
    36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, //  3  C1
    48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, //  4  C2
    60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, //  5  C3
    72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, //  6  C4
    84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, //  7  C5
    96, 97, 98, 99,100,101,102,103,104,105,106,107, //  8  C6
   108,109,110,111,112,113,114,115,116,117,118,119, //  9  C7
   120,121,122,123,124,125,126,127                  // 10
*/

// DRUMS
// C3, D3, E3, F3, G3, A3, B3
// const unsigned char note_numbers[] = {
//    60, 62, 64, 65, 67, 69, 71, 72, 72
// };

// BASS
// C1, C2, F1, G1, D2, F2, G2, E2, C3
// const unsigned char note_numbers[] = {
//    36, 48, 41, 43, 50, 53, 55, 52, 60
// };

// SOLO
// С3, F3, G3, A3, C4, F4, G4, B4, C5
const unsigned char note_numbers[] = {
   60, 65, 67, 69, 72, 77, 79, 83, 84
};


// const unsigned char note_numbers[] = {
// //   C   D   E   F   G   A   B   C
//     36, 38, 40, 41, 43, 45, 47, 48, //  3  C1
//     48, 50, 52, 53, 55, 57, 59, 60, //  4  C2
//     60, 62, 64, 65, 67, 69, 71, 72, //  5  C3
//     72, 74, 76, 77, 79, 81, 83, 84  //  6  C4
// };


const int note_velocity = 127;
const int midi_channel = 1;

const int notes_minimum_pressed_millis = 20;

#if CYCLE_OCTAVES
  const int note_sensors_amount = sensors_amount - 1;
  const int octave_switch_sensor = sensors_amount - 1;

  const int octave_cycle_repeat_delay = 750;
  const int octaves_amount = sizeof(note_numbers) / sizeof(unsigned char) / note_sensors_amount;
#else
  const int note_sensors_amount = sensors_amount;
#endif

int current_octave = 0;


MIDI_CREATE_DEFAULT_INSTANCE();

#if DEBUG_SENSORS
  long max_capacitance[sensors_amount];
#endif

void setup() {
  MIDI.begin();
  Serial.begin(115200);

  for (int i = 0; i < sensors_amount; i++) {
    sensors[i].set_CS_Timeout_Millis(sensors_timeout);
    alarmed_sensors[i] = 0;

    #if DEBUG_SENSORS
      max_capacitance[i] = 0;
    #endif
  }
}

void loop() {
  long capacitance[sensors_amount];

  unsigned long start = millis();
  if (start == 0) start = 1;

  for (int i = 0; i < sensors_amount; i = i + 1) {
    capacitance[i] = sensors[i].capacitiveSensor(sensors_samples);
  }

  #if DEBUG_SENSORS

    Serial.print(millis() - start);
    for (int i = 0; i < sensors_amount; i++) {
      if (max_capacitance[i] < capacitance[i]) max_capacitance[i] = capacitance[i];

      Serial.print("\t");
      Serial.print(capacitance[i]);
    }
    Serial.print("\n");

    Serial.print("MAX");
    for (int i = 0; i < sensors_amount; i++) {
      Serial.print("\t(");
      Serial.print(max_capacitance[i]);
      Serial.print(")");
    }
    Serial.print("\n");

  #else

    for (int i = 0; i < note_sensors_amount; i++) {
      if (capacitance[i] > sensors_alarm_threshold || capacitance[i] == -2) {
        if (alarmed_sensors[i] == 0) {
          alarmed_sensors[i] = start;
          MIDI.sendNoteOn(note_numbers[i + (note_sensors_amount * current_octave)], note_velocity, midi_channel);
        }
      } else {
        if (alarmed_sensors[i] != 0 && alarmed_sensors[i] < (start - notes_minimum_pressed_millis)) {
          alarmed_sensors[i] = 0;
          MIDI.sendNoteOff(note_numbers[i + (note_sensors_amount * current_octave)], 0, midi_channel);
        }
      }
    }

    #if CYCLE_OCTAVES
      if (capacitance[octave_switch_sensor] > sensors_alarm_threshold || capacitance[octave_switch_sensor] == -2) {
        if (alarmed_sensors[octave_switch_sensor] == 0 ||
            alarmed_sensors[octave_switch_sensor] < (start - octave_cycle_repeat_delay)
           ) {
          alarmed_sensors[octave_switch_sensor] = start;
          cycle_octaves();
        }
      } else if (alarmed_sensors[octave_switch_sensor] != 0 && alarmed_sensors[octave_switch_sensor] < (start - octave_cycle_repeat_delay)) {
        alarmed_sensors[octave_switch_sensor] = 0;
      }
    #endif

  #endif
}

#if CYCLE_OCTAVES
  void cycle_octaves() {
    for (int i = 0; i < note_sensors_amount; i++) {
      if (alarmed_sensors[i] != 0) {
        alarmed_sensors[i] = 0;
        MIDI.sendNoteOff(note_numbers[i + (note_sensors_amount * current_octave)], 0, midi_channel);
      }
    }

    current_octave++;
    if (current_octave == octaves_amount) current_octave = 0;
  }
#endif
