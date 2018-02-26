# osc2midi

simple redirect [osc](osc) midi messages to [MIDI](MIDI)  osc2midi based on oscdump [liblo](liblo)

## get source 

`git clone https://github.com/hyphop/osc2midi`

## how to BUILD osc2midi

just `./build` or `gcc -o bin/osc2midi src/osc2midi.c -llo -lpthread`

## DEPS

* liblo

[osc]: https://en.wikipedia.org/wiki/Open_Sound_Control
[MIDI]: https://en.wikipedia.org/wiki/MIDI
[liblo]: https://github.com/radarsat1/liblo
