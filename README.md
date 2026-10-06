# MyaowNES - NES Emulator

NES emulator written in C, aimed at cycle-accuracy and passing tests.

- [MyaowNES online](https://makryl.github.io/myaownes/)
- [Download release](https://github.com/makryl/myaownes/releases)
- [Zlib license](LICENSE.txt)

This project was started for educational purposes, to learn bytecode and assembly,
but at some point I just couldn't stop trying to pass every test rom I could find.

Emulator has SDL3 frontend, and the core library can be built independently.
I tried to keep code straightforward and compact,
but while the core library code was written carefully, frontend (main.c) is a bit of a mess.

It passing more than 300 test roms, but still lacks manual testing and may contain simple bugs.
Many mappers tested simply by launching some game, some mappers were not tested at all.

Almost all information I got from next sites, source code of tests and other emulators:

- [nesdev.org](https://www.nesdev.org/)
- [dendy.migera.ru](http://dendy.migera.ru/)
- [nes-test-roms](https://github.com/christopherpow/nes-test-roms), my [fork](https://github.com/makryl/nes-test-roms/tree/add-more-tests) with more tests from forums
- [MesenTests](https://github.com/nesdev-org/MesenTests)
- [AccuracyCoin](https://github.com/100thCoin/AccuracyCoin)
- [nes-audio-tests](https://github.com/bbbradsmith/nes-audio-tests)
- [little-things-nes](https://github.com/pinobatch/little-things-nes)
- [Bisqwit's NES emulator](https://bisqwit.iki.fi/source/nes.html) - very compact and easy to learn, but incomplete
- [QuickNES](https://github.com/libretro/QuickNES_Core)
- [MesenCE](https://github.com/nesdev-org/MesenCE)

## TODO

Some ideas and reminders for future:

- Fix mappers save state
- Review performance
- Extra audio in mappers
- More homebrew mappers
- Network play
- Embeded roms and saves manager
- Zapper
- NSF player mode
- Fake stereo
- Wide screen
- No sprite limit
- Slowdown
- Rewind
- CRT shader
- More tests:
  - https://www.nesdev.org/wiki/Emulator_tests
    - mmc5test_v2, mmc5ramsize
    - zapper
    - allpads
  - https://github.com/nesdev-org/MesenTests/tree/main/NES
    - sprite_evaluation_test(2), sprite_evaluation_test
  - https://github.com/bbbradsmith/nes-audio-tests
  - https://github.com/pinobatch/little-things-nes

## License

This software distributed under Zlib license: [LICENSE.txt](LICENSE.txt).

Copyright (C) 2026-2026 Maksim Krylosov <aequiternus@gmail.com>
