package fr.meudo.intercom;

import java.util.UUID;

/** Contrat BLE avec l'intercom : identique à etape-1-musique/include/ble_protocol.h. */
final class BleProtocol {
    private BleProtocol() { }

    private static UUID u(String head) { return UUID.fromString(head + "-2763-4123-8f80-5a3b7b30e82c"); }

    static final UUID SERVICE = u("8a5c0001");
    static final UUID DATA = u("8a5c0002");
    static final UUID MEDIA = u("8a5c0003");
    static final UUID INFO = u("8a5c0004");
    static final UUID COMMAND = u("8a5c0005");
    static final UUID OTA_CTRL = u("8a5c0006");
    static final UUID OTA_DATA = u("8a5c0007");
    static final UUID CCCD = UUID.fromString("00002902-0000-1000-8000-00805f9b34fb");

    static final int FLAG_TEL = 0x01, FLAG_PLAYING = 0x02, FLAG_AVRCP = 0x04, FLAG_BUTTONS = 0x08, FLAG_OTA = 0x10;

    /** Paquet DATA version 1 (32 octets, little-endian). */
    static final class Data {
        int version, flags, volume, phoneVolume, sampleRate, rssi, autoSleepMin;
        long uptimeS, freeHeap, minFreeHeap, audioBytes, positionMs, mediaCounter;

        boolean has(int flag) { return (flags & flag) != 0; }

        static Data parse(byte[] p) {
            if (p == null || p.length < 32 || (p[0] & 0xFF) != 1) return null;
            Data d = new Data();
            d.version = u8(p, 0);
            d.flags = u8(p, 1);
            d.volume = u8(p, 2);
            d.phoneVolume = u8(p, 3);
            d.sampleRate = u16(p, 4);
            d.uptimeS = u32(p, 6);
            d.freeHeap = u32(p, 10);
            d.minFreeHeap = u32(p, 14);
            d.audioBytes = u32(p, 18);
            d.positionMs = u32(p, 22);
            d.rssi = p[26];
            d.autoSleepMin = u8(p, 27);
            d.mediaCounter = u32(p, 28);
            return d;
        }
    }

    private static int u8(byte[] p, int i) { return p[i] & 0xFF; }
    private static int u16(byte[] p, int i) { return u8(p, i) | (u8(p, i + 1) << 8); }
    private static long u32(byte[] p, int i) { return (u16(p, i) | ((long) u16(p, i + 2) << 16)) & 0xFFFFFFFFL; }
}
