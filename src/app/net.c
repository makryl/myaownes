#include "net.h"

#include "app.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

enum
{
  APP_NET_PACKET_TYPE_NONE = 0,
  APP_NET_PACKET_TYPE_VERSION,
  APP_NET_PACKET_TYPE_SERVER_ROM,
  APP_NET_PACKET_TYPE_SERVER_STATE,
  APP_NET_PACKET_TYPE_SERVER_INPUT,
  APP_NET_PACKET_TYPE_CLIENT_STATE,
  APP_NET_PACKET_TYPE_CLIENT_READY,
  APP_NET_PACKET_TYPE_CLIENT_INPUT,
};

enum
{
  APP_NET_CLIENT_INPUT_PACKET_SIZE = 1 + 4 + 4 + APP_NET_INPUT_SIZE * 2,
  APP_NET_SERVER_INPUT_PACKET_SIZE = 1 + 4 + 4 + APP_NET_INPUT_SIZE * 4,
};

#define APP_NET_TRACE 0
#define APP_NET_TRACE_INPUT 0
#define APP_NET_ERROR 1

#if APP_NET_TRACE
static const char* app_net_trace_state_name[6] = { "DISCONNECTED", "CONNECTING", "SYNC_ROM",
                                                   "SYNC_STATE",   "SYNC_WAIT",  "PLAY" };
#define app_net_trace(fmt, ...) printf("NET frame=%-10d  " fmt "\n", mn_frame_num() __VA_OPT__(, ) __VA_ARGS__)
#else
#define app_net_trace(fmt, ...) (void)0
#endif

#if APP_NET_TRACE_INPUT
#define app_net_trace_input(fmt, ...) app_net_trace(fmt __VA_OPT__(, ) __VA_ARGS__)
#else
#define app_net_trace_input(fmt, ...) (void)0
#endif

#if APP_NET_ERROR
#define app_net_error(fmt, ...) \
  fprintf(stderr, "NET state=%d frame=%d  " fmt "\n", imp.net_state, mn_frame_num() __VA_OPT__(, ) __VA_ARGS__)
#else
#define app_net_error(fmt, ...) (void)0
#endif

void app_net_state(uint state)
{
  if (imp.net_state != state) {
    imp.net_state = state;
    app_net_trace("%s", app_net_trace_state_name[state]);
  }
}

static void app_net_send_schedule(const void* data, uint size)
{
  uint expected_size = imp.net_tx_scheduled + size;
  if (expected_size > imp.net_tx_size) {
    u8* buf = malloc(expected_size);
    if (imp.net_tx) {
      memcpy(buf, imp.net_tx, imp.net_tx_size);
      free(imp.net_tx);
    }
    imp.net_tx = buf;
    imp.net_tx_size = expected_size;
  }
  memcpy(imp.net_tx + imp.net_tx_scheduled, data, size);
  imp.net_tx_scheduled = expected_size;
}

static void app_net_send_stream()
{
  if (imp.net_tx_scheduled > 0) {
    uint sent = net_send_stream(imp.net_tx, imp.net_tx_scheduled);
    imp.net_tx_scheduled -= sent;
    memmove(imp.net_tx, imp.net_tx + sent, imp.net_tx_scheduled);
  }
}

static bool app_net_send_packet(u8 type)
{
  switch (type) {
    case APP_NET_PACKET_TYPE_VERSION: {
      uint version_size = sizeof(APP_VERSION) - 1;
      app_net_send_schedule(&type, sizeof(type));
      app_net_send_schedule(&version_size, sizeof(version_size));
      app_net_send_schedule(APP_VERSION, version_size);
      break;
    }
    case APP_NET_PACKET_TYPE_SERVER_ROM: {
      if (!imp.rom) {
        return false;
      }
      uint rom_size = sizeof(struct mn_rom) + imp.rom->prg_rom_size + imp.rom->chr_rom_size + imp.rom->prg_ram_size
                    + imp.rom->chr_ram_size;
      app_net_send_schedule(&type, sizeof(type));
      app_net_send_schedule(&rom_size, sizeof(rom_size));
      app_net_send_schedule(imp.rom, rom_size);
      break;
    }
    case APP_NET_PACKET_TYPE_SERVER_STATE: {
      uint state_size = mn_save_size();
      void* data = app_rewind_data(1);
      if (!data) {
        return false;
      }
      app_net_send_schedule(&type, sizeof(type));
      app_net_send_schedule(&imp.net_sync, sizeof(imp.net_sync));
      app_net_send_schedule(&state_size, sizeof(state_size));
      app_net_send_schedule(data, state_size);
      break;
    }
    case APP_NET_PACKET_TYPE_SERVER_INPUT: {
      static_assert(APP_NET_SERVER_INPUT_PACKET_SIZE <= sizeof(imp.net_server_input_packet));
      uint frame = mn_frame_num();
      uint input = (imp.net_joy1 & 0xFFFF) | (imp.net_joy2 << 16);
      imp.net_server_input_packet[0] = type;
      memcpy(imp.net_server_input_packet + 1, &imp.net_sync, 4);
      memcpy(imp.net_server_input_packet + 5, &frame, 4);
      memcpy(imp.net_server_input_packet + (9 + (frame % APP_NET_INPUT_SIZE) * 4), &input, 4);
      net_send_datagram(imp.net_server_input_packet, APP_NET_SERVER_INPUT_PACKET_SIZE);
      break;
    }
    case APP_NET_PACKET_TYPE_CLIENT_INPUT: {
      static_assert(APP_NET_CLIENT_INPUT_PACKET_SIZE <= sizeof(imp.net_client_input_packet));
      uint frame = mn_frame_num();
      u16 input = imp.pause ? 0 : imp.joy1;
      imp.net_client_input_packet[0] = type;
      memcpy(imp.net_client_input_packet + 1, &imp.net_sync, 4);
      memcpy(imp.net_client_input_packet + 5, &frame, 4);
      memcpy(imp.net_client_input_packet + (9 + (frame % APP_NET_INPUT_SIZE) * 2), &input, 2);
      net_send_datagram(imp.net_client_input_packet, APP_NET_CLIENT_INPUT_PACKET_SIZE);
      break;
    }
    case APP_NET_PACKET_TYPE_CLIENT_STATE:
    case APP_NET_PACKET_TYPE_CLIENT_READY: {
      app_net_send_schedule(&type, sizeof(type));
      break;
    }
  }
  return true;
}

static uint app_net_recv_packet(u8* packet, uint packet_size)
{
  switch (packet[0]) {
    case APP_NET_PACKET_TYPE_VERSION: {
      if (packet_size < 5) {
        return 5;
      }
      uint version_size = 0;
      memcpy(&version_size, packet + 1, 4);
      if (version_size != sizeof(APP_VERSION) - 1) {
        app_net_error("version mismatch");
        net_disconnect();
        return 0;
      }
      uint total_size = 5 + version_size;
      if (packet_size < total_size) {
        return total_size;
      }
      char version[sizeof(APP_VERSION)];
      memcpy(version, packet + 5, version_size);
      version[sizeof(version) - 1] = 0;
      if (strcmp(version, APP_VERSION) != 0) {
        app_net_error("version mismatch");
        net_disconnect();
        return 0;
      }
      return 0;
    }
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
      imp.net_sync = 0;
      mn_rom_set(imp.rom);
      app_rom_update();
      app_net_send_packet(APP_NET_PACKET_TYPE_CLIENT_STATE);
      app_net_state(APP_NET_STATE_SYNC_WAIT);
      return 0;
    }
    case APP_NET_PACKET_TYPE_SERVER_STATE: {
      if (packet_size < 9) {
        return 9;
      }
      uint packet_sync;
      uint state_size;
      memcpy(&packet_sync, packet + 1, 4);
      memcpy(&state_size, packet + 5, 4);
      uint total_size = 9 + state_size;
      if (packet_size < total_size) {
        return total_size;
      }
      if (!mn_load_data(packet + 9, state_size)) {
        // todo: reconnect?
        app_net_error("state load error");
        net_disconnect();
        return 0;
      }
      imp.net_sync = packet_sync;
      imp.net_frame = mn_frame_num();
      app_net_send_packet(APP_NET_PACKET_TYPE_CLIENT_READY);
      app_net_state(APP_NET_STATE_PLAY);
      return 0;
    }
    case APP_NET_PACKET_TYPE_CLIENT_STATE: {
      ++imp.net_sync;
      app_net_state(APP_NET_STATE_SYNC_STATE);
      break;
    }
    case APP_NET_PACKET_TYPE_CLIENT_READY: {
      app_net_state(APP_NET_STATE_PLAY);
      imp.net_frame = mn_frame_num();
      return 0;
    }
    case APP_NET_PACKET_TYPE_CLIENT_INPUT: {
      if (packet_size < APP_NET_CLIENT_INPUT_PACKET_SIZE) {
        return 0;
      }
      uint packet_sync;
      uint packet_frame;
      u16 packet_input[APP_NET_INPUT_SIZE];
      memcpy(&packet_sync, packet + 1, sizeof(packet_sync));
      memcpy(&packet_frame, packet + 5, sizeof(packet_frame));
      memcpy(packet_input, packet + 9, sizeof(packet_input));
      if (packet_sync != imp.net_sync) {
        return 0;
      }
      if (packet_frame <= imp.net_frame) {
        return 0;
      }
      uint local_frame = mn_frame_num();
      if (packet_frame > local_frame) {
        uint local_diff = packet_frame - local_frame;
        if (local_diff > APP_NET_INPUT_SIZE) {
          app_net_trace("lag local %d", local_diff);
          ++imp.net_sync;
          app_net_state(APP_NET_STATE_SYNC_STATE);
          return 0;
        }
      }
      uint net_diff = packet_frame - imp.net_frame;
      if (net_diff > APP_NET_INPUT_SIZE) {
        // todo: sync?
        app_net_trace("lag %d", net_diff);
        imp.net_frame = packet_frame - APP_NET_INPUT_SIZE;
      }
      // uint joy2 = packet_input[APP_NET_INPUT_SIZE - 1];
      uint joy2 = 0;
      for (uint net_frame = imp.net_frame + 1; net_frame <= packet_frame; ++net_frame) {
        joy2 |= packet_input[net_frame % APP_NET_INPUT_SIZE];
      }
      if (imp.net_joy2 != joy2) {
        imp.net_joy2 = joy2;
        app_net_trace_input("input %04X %d %s", joy2, net_diff, net_diff > 1 ? "merged" : "");
      }
      imp.net_frame = packet_frame;
      return 0;
    }
    case APP_NET_PACKET_TYPE_SERVER_INPUT: {
      if (packet_size < APP_NET_SERVER_INPUT_PACKET_SIZE) {
        return 0;
      }
      uint packet_sync;
      uint packet_frame;
      uint packet_input[APP_NET_INPUT_SIZE];
      memcpy(&packet_sync, packet + 1, sizeof(packet_sync));
      memcpy(&packet_frame, packet + 5, sizeof(packet_frame));
      memcpy(packet_input, packet + 9, sizeof(packet_input));
      if (packet_sync != imp.net_sync) {
        return 0;
      }
      if (packet_frame <= imp.net_frame) {
        return 0;
      }
      uint net_diff = packet_frame - imp.net_frame;
      if (net_diff > APP_NET_INPUT_SIZE) {
        app_net_trace("lag %d", net_diff);
        imp.net_sync = 0;
        app_net_send_packet(APP_NET_PACKET_TYPE_CLIENT_STATE);
        app_net_state(APP_NET_STATE_SYNC_WAIT);
        return 0;
      }
      for (uint net_frame = imp.net_frame + 1; net_frame <= packet_frame; ++net_frame) {
        uint net_input = packet_input[net_frame % APP_NET_INPUT_SIZE];
        uint local_frame = mn_frame_num();
        if (net_frame > local_frame) {
          uint local_diff = net_frame - local_frame;
          app_net_trace("ffwd %d", local_diff);
          for (uint j = 0; j < local_diff; ++j) {
            app_rewind_save();
            mn_frame_fast();
          }
        } else if (net_frame < local_frame) {
          uint local_diff = local_frame - net_frame;
          uint local_input = imp.net_input[net_frame % APP_NET_INPUT_SIZE];
          if (local_input == net_input) {
            continue;
          }
          app_net_trace("rwnd %d", local_diff);
          if (!app_rewind_load(local_diff)) {
            app_net_trace("rwnd error");
            imp.net_sync = 0;
            app_net_send_packet(APP_NET_PACKET_TYPE_CLIENT_STATE);
            app_net_state(APP_NET_STATE_SYNC_WAIT);
            return 0;
          }
        }
        uint joy1 = (net_input & 0xFFFF);
        uint joy2 = (net_input >> 16);
        mn_input(joy1, joy2);
        imp.net_input[net_frame % APP_NET_INPUT_SIZE] = net_input;
        if (imp.net_joy1 != joy1 || imp.net_joy2 != joy2) {
          imp.net_joy1 = joy1;
          imp.net_joy2 = joy2;
          app_net_trace_input("input %04X %04X", joy1, joy2);
        }
      }
      imp.net_frame = packet_frame;
      return 0;
    }
  }
  return 0;
}

static void app_net_recv_stream()
{
  uint received = 0;
  do {
    if (imp.net_rx_expected == 0) {
      imp.net_rx_received = 0;
      imp.net_rx_expected = 1;
    }
    if (imp.net_rx_expected > imp.net_rx_size) {
      u8* buf = malloc(imp.net_rx_expected);
      if (imp.net_rx) {
        if (imp.net_rx_received) {
          memcpy(buf, imp.net_rx, imp.net_rx_received);
        }
        free(imp.net_rx);
      }
      imp.net_rx = buf;
      imp.net_rx_size = imp.net_rx_expected;
    }
    received = net_recv_stream(imp.net_rx + imp.net_rx_received, imp.net_rx_expected - imp.net_rx_received);
    if (received > 0) {
      imp.net_rx_received += received;
      imp.net_rx_expected = app_net_recv_packet(imp.net_rx, imp.net_rx_received);
    }
  } while (received);
}

static void app_net_recv_datagram()
{
  uint received = 0;
  do {
    received = net_recv_datagram(&imp.net_packet, sizeof(imp.net_packet));
    if (received > 0) {
      app_net_recv_packet(imp.net_packet, received);
    }
  } while (received);
}

void app_net_update()
{
  switch (net_update()) {
    case NET_STATE_DISCONNECTED:
      switch (imp.net_state) {
        case APP_NET_STATE_DISCONNECTED:
          if (imp.net_is_server || imp.net_is_client) {
            app_net_state(APP_NET_STATE_CONNECTING);
          }
          break;
        case APP_NET_STATE_CONNECTING:
          if (imp.net_is_server) {
            net_server(imp.net_address, imp.net_port == 0 ? APP_NET_DEFAULT_PORT : imp.net_port);
          } else if (imp.net_is_client) {
            net_client(imp.net_address, imp.net_port == 0 ? APP_NET_DEFAULT_PORT : imp.net_port);
          } else {
            app_popup_bottom("Disconnected", 2);
            app_net_state(APP_NET_STATE_DISCONNECTED);
          }
          break;
        case APP_NET_STATE_SYNC_ROM:
        case APP_NET_STATE_SYNC_STATE:
        case APP_NET_STATE_PLAY:
          if (imp.net_is_client) {
            app_rom_unload();
            app_rom_update();
          }
          app_net_state(APP_NET_STATE_CONNECTING);
          break;
      }
      break;
    case NET_STATE_CONNECTING: app_popup_bottom("Connecting", 0.5); break;
    case NET_STATE_CONNECTED:
      switch (imp.net_state) {
        case APP_NET_STATE_DISCONNECTED:
        case APP_NET_STATE_CONNECTING:
          if (imp.net_is_server) {
            app_net_send_packet(APP_NET_PACKET_TYPE_VERSION);
            app_net_state(APP_NET_STATE_SYNC_ROM);
          } else {
            app_net_state(APP_NET_STATE_SYNC_WAIT);
          }
          break;
        case APP_NET_STATE_SYNC_ROM:
          app_popup_bottom("Sync ROM", 0.5);
          if (app_net_send_packet(APP_NET_PACKET_TYPE_SERVER_ROM)) {
            app_net_state(APP_NET_STATE_SYNC_WAIT);
          }
          break;
        case APP_NET_STATE_SYNC_STATE:
          app_popup_bottom("Sync state", 0.5);
          if (app_net_send_packet(APP_NET_PACKET_TYPE_SERVER_STATE)) {
            app_net_state(APP_NET_STATE_SYNC_WAIT);
          }
          break;
        case APP_NET_STATE_SYNC_WAIT: app_popup_bottom("Sync", 0.5);
        case APP_NET_STATE_PLAY:
          app_net_recv_stream();
          app_net_recv_datagram();
          app_net_send_stream();
          break;
      }
      break;
  }
}

bool app_net_input()
{
  switch (imp.net_state) {
    case APP_NET_STATE_SYNC_STATE:
    case APP_NET_STATE_SYNC_WAIT: return false;
    case APP_NET_STATE_PLAY:
      if (imp.net_is_server) {
        imp.net_joy1 = imp.pause ? 0 : imp.joy1;
        app_net_send_packet(APP_NET_PACKET_TYPE_SERVER_INPUT);
        mn_input(imp.net_joy1, imp.net_joy2);
      } else {
        if (mn_frame_num() >= imp.net_frame + APP_NET_INPUT_SIZE) {
          return false;
        }
        app_net_send_packet(APP_NET_PACKET_TYPE_CLIENT_INPUT);
        mn_input(imp.net_joy1, imp.net_joy2);
        imp.net_input[mn_frame_num() % APP_NET_INPUT_SIZE] = (imp.net_joy1 & 0xFFFF) | (imp.net_joy2 << 16);
      }
      break;
  }
  return true;
}
