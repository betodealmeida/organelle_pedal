/* A MIDI pedal for the Critter & Guitari Organelle.
 *
 * An expression pedal sends CC numbers 21-24, depending on
 * the switch pressed. A sustain pedal sends CC 25 to control
 * the Aux button.
 */

#include <Bounce.h>

#define LOWER 0
#define HIGHER 1

const int midiChannel = 15;
const int expressionCC[] = {21, 22, 23, 24};
const int sustainCC = 25;
Bounce switches[] = {
    Bounce(5, 10),
    Bounce(6, 10),
    Bounce(7, 10),
    Bounce(8, 10),
};

/* When we switch the knob beinf affected the expression pedal
 * doesn't affect it immediately. Instead, there's a target value
 * that need to be hit.
 *
 * For example, if knob is at 50 and we switch to it when the
 * expression pedal is at 33, the value will only change when the
 * expression pedal reads 50 or more (HIGHER).
 */
int targetValues[] = {0, 0, 0, 0};
int targetSide = HIGHER;
bool sendingCC = false;

const int sustainPin = 15;
Bounce sustainPedal = Bounce(sustainPin, 10); // 10 ms debounce

int activeKnob = -1;
int i, pinNumber;

void setup() {
  pinMode(sustainPin, INPUT_PULLUP);

  // set pullup in switches
  for (pinNumber = 5; pinNumber < 9; pinNumber++) {
    pinMode(pinNumber, INPUT_PULLUP);
  }

  // set LED pins as outputs
  for (pinNumber = 9; pinNumber < 13; pinNumber++) {
    pinMode(pinNumber, OUTPUT);
  }
}

void loop() {
  // read sustain pedal
  if (sustainPedal.update()) {
    // signal goes HIGH when pedal is pressed
    if (sustainPedal.risingEdge()) {
      usbMIDI.sendControlChange(sustainCC, 127, midiChannel);
    } else if (sustainPedal.fallingEdge()) {
      usbMIDI.sendControlChange(sustainCC, 0, midiChannel);
    }
  }

  // read expression pedal
  int expressionValue = analogRead(A0) / 8;

  // are we active?
  if (activeKnob != -1) {
    if (sendingCC) {
      if (expressionValue != targetValues[activeKnob]) {
        usbMIDI.sendControlChange(expressionCC[activeKnob], expressionValue,
                                  midiChannel);
        targetValues[activeKnob] = expressionValue;
      }
    } else if (((targetSide == HIGHER) &&
                (expressionValue >= targetValues[activeKnob])) ||
               ((targetSide == LOWER) &&
                (expressionValue <= targetValues[activeKnob]))) {
      sendingCC = true;
    }
  }

  for (i = 0; i < 4; i++) {
    // read switches
    if (switches[i].update() && switches[i].fallingEdge()) {
      activeKnob = activeKnob != i ? i : -1;
      sendingCC = false;

      if (activeKnob != -1) {
        if (expressionValue < targetValues[activeKnob]) {
          targetSide = HIGHER;
        } else if (expressionValue > targetValues[activeKnob]) {
          targetSide = LOWER;
        }
      }
    }

    // turn active knob LED on
    digitalWrite(9 + i, activeKnob == i ? HIGH : LOW);
  }

  // discard incoming MIDI messages
  while (usbMIDI.read()) {
  }
  delay(5);
}
