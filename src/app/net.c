#include "net.h"

#include "app.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

enum
{
  APP_NET_PACKET_TYPE_SERVER_ROM,
  APP_NET_PACKET_TYPE_SERVER_STATE,
  APP_NET_PACKET_TYPE_SERVER_INPUT,
  APP_NET_PACKET_TYPE_CLIENT_SYNC,
  APP_NET_PACKET_TYPE_CLIENT_READY,
  APP_NET_PACKET_TYPE_CLIENT_INPUT,
};

#define app_net_trace(fmt, ...) \
  printf("NET state=%d frame=%d  " fmt "\n", imp.net_state, imp.frame __VA_OPT__(, ) __VA_ARGS__)

void app_net_state(uint state)
{
  if (imp.net_state != state) {
    imp.net_state = state;
    app_net_trace("");
  }
}

static void app_net_send_packet(u8 type)
{
  app_net_send(&type, sizeof(type));
  switch (type) {
    case APP_NET_PACKET_TYPE_SERVER_ROM: {
      uint rom_size = sizeof(struct mn_rom) + imp.rom->prg_rom_size + imp.rom->chr_rom_size + imp.rom->prg_ram_size
                    + imp.rom->chr_ram_size;
      app_net_send(&rom_size, sizeof(rom_size));
      app_net_send(imp.rom, rom_size);
      break;
    }
    case APP_NET_PACKET_TYPE_SERVER_STATE: {
      uint frame = imp.frame - 1;
      uint state_size = mn_save_size();
      app_net_send(&frame, sizeof(frame));
      app_net_send(&state_size, sizeof(state_size));
      app_net_send(app_rewind_data(1), state_size);
      break;
    }
    case APP_NET_PACKET_TYPE_SERVER_INPUT: {
      u16 joy1 = imp.joy1;
      u16 joy2 = imp.net_joy2;
      app_net_send(&imp.frame, sizeof(imp.frame));
      app_net_send(&joy1, sizeof(joy1));
      app_net_send(&joy2, sizeof(joy2));
      app_net_trace("send %04X %04X", joy1, joy2);
      break;
    }
    case APP_NET_PACKET_TYPE_CLIENT_INPUT: {
      u16 joy1 = imp.joy1;
      app_net_send(&imp.frame, sizeof(imp.frame));
      app_net_send(&joy1, sizeof(joy1));
      break;
    }
    case APP_NET_PACKET_TYPE_CLIENT_SYNC:
    case APP_NET_PACKET_TYPE_CLIENT_READY: {
      break;
    }
  }
}

static uint app_net_recv_packet(u8* packet, uint packet_size)
{
  switch (packet[0]) {
    case APP_NET_PACKET_TYPE_SERVER_ROM: {
      if (packet_size < 5) {
        return 5;
      }
      uint rom_size = 0;
      memcpy(&rom_size, packet + 1, 4);
      uint total_size = 5 + rom_size;
      if (packet_size < total_size) {
        return total_size;
      }
      u8* data = malloc(rom_size);
      memcpy(data, packet + 5, rom_size);
      app_tas_unload();
      app_rom_unload();
      imp.rom = (mn_rom)data;
      u8* prg_rom = data + sizeof(struct mn_rom);
      u8* chr_rom = prg_rom + imp.rom->prg_rom_size;
      u8* prg_ram = chr_rom + imp.rom->chr_rom_size;
      u8* chr_ram = prg_ram + imp.rom->prg_ram_size;
      imp.rom->prg_rom = prg_rom;
      imp.rom->chr_rom = imp.rom->chr_rom_size > 0 ? chr_rom : nullptr;
      imp.rom->prg_ram = imp.rom->prg_ram_size > 0 ? prg_ram : nullptr;
      imp.rom->chr_ram = imp.rom->chr_ram_size > 0 ? chr_ram : nullptr;
      mn_rom_set(imp.rom);
      app_rom_update();
      app_net_send_packet(APP_NET_PACKET_TYPE_CLIENT_SYNC);
      app_net_state(APP_NET_STATE_SYNC);
      return 0;
    }
    case APP_NET_PACKET_TYPE_SERVER_STATE: {
      if (packet_size < 9) {
        return 9;
      }
      uint frame = 0;
      uint state_size = 0;
      memcpy(&frame, packet + 1, 4);
      memcpy(&state_size, packet + 5, 4);
      uint total_size = 9 + state_size;
      if (packet_size < total_size) {
        return total_size;
      }
      if (state_size != mn_save_size()) {
        app_net_client_close();
        return 0;
      }
      mn_load_data(packet + 9);
      app_net_send_packet(APP_NET_PACKET_TYPE_CLIENT_READY);
      app_net_state(APP_NET_STATE_PLAY);
      imp.frame = frame;
      return 0;
    }
    case APP_NET_PACKET_TYPE_CLIENT_SYNC: {
      app_net_send_packet(APP_NET_PACKET_TYPE_SERVER_STATE);
      app_net_state(APP_NET_STATE_SYNC);
      break;
    }
    case APP_NET_PACKET_TYPE_CLIENT_READY: {
      app_net_state(APP_NET_STATE_PLAY);
      return 0;
    }
    case APP_NET_PACKET_TYPE_CLIENT_INPUT: {
      if (packet_size < 7) {
        return 7;
      }
      uint frame = 0;
      u16 joy2 = 0;
      memcpy(&frame, packet + 1, 4);
      memcpy(&joy2, packet + 5, 2);
      imp.net_joy2 = joy2;
      return 0;
    }
    case APP_NET_PACKET_TYPE_SERVER_INPUT: {
      if (packet_size < 9) {
        return 9;
      }
      uint frame = 0;
      u16 joy1 = 0;
      u16 joy2 = 0;
      memcpy(&frame, packet + 1, 4);
      memcpy(&joy1, packet + 5, 2);
      memcpy(&joy2, packet + 7, 2);
      if (frame > imp.frame) {
        uint diff = frame - imp.frame;
        // todo: sync on too much lag
        for (uint i = 0; i < diff; ++i) {
          mn_frame_fast();
        }
        app_net_trace("ffwd %d", frame);
      } else if (frame < imp.frame) {
        uint diff = imp.frame - frame;
        // todo: sync on too much lag
        app_rewind_load(diff);
        app_net_trace("rwnd %d", frame);
      }
      imp.frame = frame;
      imp.net_joy1 = joy1;
      imp.net_joy2 = joy2;
      mn_input(joy1, joy2);
      app_net_trace("recv %04X %04X", joy1, joy2);
      return 0;
    }
  }
  return 0;
}

static void app_net_send_input()
{
  if (imp.net_is_server) {
    if (imp.joy1 != imp.net_sent_joy1 || imp.net_joy2 != imp.net_sent_joy2) {
      app_net_send_packet(APP_NET_PACKET_TYPE_SERVER_INPUT);
      imp.net_sent_joy1 = imp.joy1;
      imp.net_sent_joy2 = imp.net_joy2;
    }
  } else {
    if (imp.joy1 != imp.net_sent_joy1) {
      app_net_send_packet(APP_NET_PACKET_TYPE_CLIENT_INPUT);
      imp.net_sent_joy1 = imp.joy1;
    }
  }
}

static void app_net_recv_stream()
{
  uint received = 0;
  do {
    if (imp.net_buf_expected == 0) {
      imp.net_buf_received = 0;
      imp.net_buf_expected = 1;
    }
    if (imp.net_buf_expected > imp.net_buf_size) {
      u8* buf = malloc(imp.net_buf_expected);
      memcpy(buf, imp.net_buf, imp.net_buf_received);
      free(imp.net_buf);
      imp.net_buf = buf;
      imp.net_buf_size = imp.net_buf_expected;
    }
    received = app_net_recv(&imp.net_buf[imp.net_buf_received], imp.net_buf_expected - imp.net_buf_received);
    if (received > 0) {
      imp.net_buf_received += received;
      imp.net_buf_expected = app_net_recv_packet(imp.net_buf, imp.net_buf_received);
    }
  } while (received);
}

void app_net_update()
{
  switch (imp.net_state) {
    case APP_NET_STATE_LISTENING: app_net_server_accept(); break;
    case APP_NET_STATE_CONNECTING: app_net_client_poll(); break;
    case APP_NET_STATE_CONNECTED:
      if (imp.net_is_server) {
        app_net_send_packet(APP_NET_PACKET_TYPE_SERVER_ROM);
      }
      app_net_state(APP_NET_STATE_SYNC);
      break;
    case APP_NET_STATE_SYNC: app_net_recv_stream(); break;
    case APP_NET_STATE_PLAY:
      app_net_recv_stream();
      app_net_send_input();
      break;
  };
  if (imp.net_is_server) {
    mn_input(imp.joy1, imp.net_joy2);
  } else if (imp.net_is_client) {
    mn_input(imp.net_joy1, imp.net_joy2);
  }
}
