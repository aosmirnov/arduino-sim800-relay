# Arduino Nano + SIM800 Relay controller

Tested on Arduino Nano v3 (CH340) + SIM800L (Z1435) + 4-channel 5Vdc relay module

## Avaliable actions:
1) Switch output pin ON
2) Switch output pin OFF
3) Switch output pin once ON and OFF with delay

## Avaliable sms commands:
1) RON + [DIGIT] - switch on relay [NUMBER], e.g. RON1 switches on relay 1 (first pin number in array below)
2) ROFF + [DIGIT] - switch off, same as RON
3) RBL + [DIGIT] - blink with adjustable delay
4) STATUS - sends sms with relay status, e.g. 1:OFF 2:OFF 3:OFF 4:ON
5) HELP - sends sms with avaliable commands, e.g. RON,ROFF,RBL + [1..4]; STATUS; HELP
Text of RON, ROFF and RBL commands is adjustable in User variables section.
Commands in sms are not case sensitive: RON1 = rOn1 = ron1, etc.
But command syntax remains the same: TEXT + DIGIT.

## Wiring diagram
