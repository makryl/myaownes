#include "myaownes.h"
#include "imp_sdl.h"
#include <unistd.h>

bool imp_init()
{
  const char* test = nullptr;

  // test = "nes-test-roms/other/nestest.nes";
  // test = "nes-test-roms/240pee/240pee.nes";
  // test = "nes-test-roms/ppu_vbl_nmi/ppu_vbl_nmi.nes";
  // test = "nes-test-roms/nmi_sync/demo_ntsc.nes";
  // test = "nes-test-roms/nmi_sync/demo_pal.nes";
  // test = "nes-test-roms/ppu_sprite_hit/rom_singles/09-timing.nes";
  // test = "nes-test-roms/ppu_sprite_overflow/rom_singles/03-timing.nes";
  // test = "nes-test-roms/tvpassfail/tv.nes";
  // test = "nes-test-roms/apu_mixer/square.nes";
  // test = "nes-test-roms/apu_mixer/triangle.nes";
  // test = "nes-test-roms/apu_mixer/noise.nes";
  // test = "nes-test-roms/apu_mixer/dmc.nes";
  // test = "nes-test-roms/dma_sync_test/dma_sync_test_odd.nes";
  // test = "nes-test-roms/dma_sync_test/dma_sync_test.nes";
  // test = "nes-test-roms/dmc_tests/buffer_retained.nes";
  // test = "nes-test-roms/dmc_tests/latency.nes";
  // test = "nes-test-roms/dmc_tests/status_irq.nes";
  // test = "nes-test-roms/dmc_tests/status.nes";
  // test = "nes-test-roms/volume_tests/volumes.nes";
  // test = "nes-test-roms/other/8bitpeoples_-_deadline_console_invitro.nes";

  // test = "GoodNES/USA/Mega Man (U) [!].nes";
  // test = "GoodNES/USA/Contra (U) [!].nes";
  // test = "GoodNES/Japan/Fire Emblem Gaiden (J) [!].nes";
  // test = "MegaPack/Translated/Fire Emblem Gaiden (J) [T-Eng97b2].nes";

  // test = "GoodNES/USA/Battletoads (U) [!].nes";
  // test = "MegaPack/USA/Micro Machines (U).nes";
  // test = "GoodNES/USA/Marble Madness (U) [!].nes";
  // test = "GoodNES/USA/Crystalis (U) [!].nes";
  // test = "GoodNES/USA/Super Mario Bros. + Duck Hunt (U) [!].nes";
  // test = "GoodNES/USA/Super Mario Bros. 3 (U) (V1.1) [!].nes";
  // test = "GoodNES/USA/Rad Racer (U) [!].nes";
  // test = "GoodNES/USA/Legend of Zelda, The (U) (V1.1) [!].nes";
  // test = "GoodNES/USA/Kirby's Adventure (U) (V1.1) [!].nes";
  // test = "GoodNES/USA/Immortal, The (U) [!].nes";

  // test = "GoodNES/Europe/Super Mario Bros. + Duck Hunt (E) [!].nes";
  // test = "GoodNES/USA/Elite (U) (Proto) [!].nes";
  // test = "GoodNES/Europe/Elite (E) [!].nes";
  // test = "GoodNES/Europe/Asterix (E) [!].nes";
  // test = "GoodNES/Europe/Smurfs, The (E) [!].nes";
  // test = "GoodNES/USA/Batman - Return of the Joker (U) [!].nes"; // FME-7
  // test = "GoodNES/Japan/Magic John (J) [!].nes"; // Jaleco SS
  // test = "GoodNES/Japan/TwinBee 3 - Poko Poko Daimaou (J) [!].nes"; // VRC2a	22 +
  // test = "GoodNES/Japan/Konami Wai Wai World (J) [!].nes"; // VRC2b	23-3 +
  // test = "GoodNES/Japan/Contra (J) [!].nes"; // VRC2b	23-3 +
  // test = "GoodNES/Japan/Ganbare Goemon Gaiden - Kieta Ougon Kiseru (J) (V1.1) [!].nes"; // VRC2c	25-3 +
  // test = "GoodNES/Japan/Wai Wai World 2 - SOS!! Paseri Jou (J) [!].nes"; // VRC4a	21-1 +
  // test = "GoodNES/Japan/Bio Miracle Bokutte Upa (J) [!].nes"; // VRC4b	25-1 +
  // test = "GoodNES/Japan/Ganbare Goemon Gaiden 2 - Tenka no Zaihou (J) [!].nes"; // VRC4c 21-2 +
  // test = "GoodNES/Japan/Teenage Mutant Ninja Turtles (J) [!].nes"; // VRC4d	25-2 +
  // test = "GoodNES/Japan/Akumajou Special - Boku Dracula-kun (J) [!].nes"; // VRC4e	23-2 +
  // test = "GoodNES/Japan/Tiny Toon Adventures (J) [!].nes"; // VRC4e	23-2 +
  // test = "GoodNES/Japan/Akumajou Densetsu (J) [!].nes"; // VRC6a +
  // test = "GoodNES/Japan/Esper Dream 2 - Aratanaru Tatakai (J) [!].nes"; // VRC6b +
  // test = "GoodNES/Japan/Mouryou Senki Madara (J) [!].nes"; // VRC6b +
  // test = "GoodNES/Japan/Salamander (J) [!].nes"; // VRC3 +
  // test = "GoodNES/Japan/Ganbare Goemon! - Karakuri Douchuu (J) [!].nes"; // VRC1 +
  // test = "GoodNES/Japan/Tetsuwan Atom (J) [!].nes"; // VRC1 +
  // test = "GoodNES/Japan/Lagrange Point (J) [!].nes"; // VRC7a +
  // test = "GoodNES/Japan/Tiny Toon Adventures 2 - Montana Land e Youkoso (J) [!].nes"; // VRC7b +
  // test = "GoodNES/Japan/Fantasy Zone II - Opa-Opa no Namida (J) [!].nes"; // Sunsoft-3
  // test = "GoodNES/Japan/Mappy Kids (J) [!].nes"; // Namco-163
  // test = "MegaPack/Translated/Splatter House - Wanpaku Graffiti (J) [T-Eng2.0].nes"; // Namco-163
  // test = "GoodNES/Japan/Digital Devil Story - Megami Tensei II (J) (V1.1) [!].nes"; // Namco-163
  // test = "GoodNES/Japan/King of Kings (J) [!].nes"; // Namco-163
  // test = "GoodNES/Japan/Star Wars (J) (Namco) [!].nes"; // Namco-163
  // test = "GoodNES/Japan/Dokuganryuu Masamune (J) [!].nes"; // Namco-163
  // test = "GoodNES/Japan/Family Circuit '91 (J) [!].nes"; // Namco-175
  // test = "GoodNES/Japan/Splatterhouse - Wanpaku Graffiti (J) [!].nes"; // Namco-340
  // test = "GoodNES/Japan/Digital Devil Story - Megami Tensei (J) [!].nes"; // Namco-3446
  // test = "GoodNES/Japan/Babel no Tou (J) (V1.0) [!].nes"; // Namco-118
  // test = "GoodNES/Japan/Family Jockey (J) [!].nes"; // Namco-118
  // test = "GoodNES/Japan/Metro-Cross (J) [!].nes"; // Namco-118
  // test = "GoodNES/Japan/Quinty (J) [!].nes"; // Namco-3433
  // test = "GoodNES/Japan/Devil Man (J) [!].nes"; // Namco-3453
  // test = "GoodNES/Japan/Dragon Buster (J) [!].nes"; // Namco-3425
  // test = "GoodNES/Japan/Akuma-kun - Makai no Wana (J) [!].nes"; // Bandai FCG-2
  // test = "GoodNES/Japan/Crayon Shin-chan - Ora to Poi Poi (J) [!].nes"; // Bandai LZ93D50
  // test = "GoodNES/Japan/Dragon Ball - Daimaou Fukkatsu (J) [!].nes"; // Bandai FCG-1
  // test = "GoodNES/Japan/Dragon Ball 3 - Gokuu Den (J) (V1.1) [!].nes"; // Bandai FCG-2
  // test = "GoodNES/Japan/Dragon Ball Z II - Gekishin Freeza!! (J) (V1.1) [!].nes"; // Bandai LZ93D50 EEPROM 256
  // test = "GoodNES/Japan/Dragon Ball Z - Kyoushuu! Saiya Jin (J) (V1.1) [!].nes"; // Bandai LZ93D50 EEPROM 128
  // test = "GoodNES/Japan/Famicom Jump II - Saikyou no 7 Nin (J) [!].nes"; // Bandai LZ93D50 WRAM
  // test = "MegaPack/USA/Klax (U).nes"; // Tengen Rambo-1
  // test = "MegaPack/USA/Skull & Crossbones (U).nes"; // Tengen Rambo-1
  // test = "MegaPack/USA/Shinobi (U).nes"; // Tengen Rambo-1
  // test = "MegaPack/USA/Alien Syndrome (U).nes"; // Tengen Rambo-1 800037 rom header bug

  if (test) {
    chdir("../../tmp");
    imp_rom_load(test);
  }

  return true;
}

void imp_quit() {}
