 // Copyright © 2023 Trystan A Larey-Williams
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#define MAX_COUNTERS 12

volatile unsigned int counters[MAX_COUNTERS];
unsigned int counterSamples[MAX_COUNTERS];

//
// ISRs to increment counters via HW pin interrrupt
//
void IRAM_ATTR IncCounter0() {
    ++counters[0];
}
void IRAM_ATTR IncCounter1() {
    ++counters[1];
}
void IRAM_ATTR IncCounter2() {
    ++counters[2];
}
void IRAM_ATTR IncCounter3() {
    ++counters[3];
}
void IRAM_ATTR IncCounter4() {
    ++counters[4];
}
void IRAM_ATTR IncCounter5() {
    ++counters[5];
}
void IRAM_ATTR IncCounter6() {
    ++counters[6];
}
void IRAM_ATTR IncCounter7() {
    ++counters[7];
}
void IRAM_ATTR IncCounter8() {
    ++counters[8];
}
void IRAM_ATTR IncCounter9() {
    ++counters[9];
}
void IRAM_ATTR IncCounter10() {
    ++counters[10];
}
void IRAM_ATTR IncCounter11() {
    ++counters[11];
}

void (*counterISRs[])(void) {
  IncCounter0,
  IncCounter1,
  IncCounter2,
  IncCounter3,
  IncCounter4,
  IncCounter5,
  IncCounter6,
  IncCounter7,
  IncCounter8,
  IncCounter9,
  IncCounter10,
  IncCounter11
};

//
// Called 'exactly' every second to update counters via timer interrupt.
//
void IRAM_ATTR counterSampler()
{
  memcpy( counterSamples, (void*)counters, sizeof( unsigned int ) * MAX_COUNTERS );
  memset( (void*)counters, 0, sizeof( unsigned int ) * MAX_COUNTERS );
}
