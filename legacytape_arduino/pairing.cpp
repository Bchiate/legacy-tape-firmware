#include "pairing.h"
#include <Preferences.h>
#include <esp_system.h>
#include <esp_random.h>
#include <bootloader_random.h>   // entropy source for the RNG before WiFi/BT start
#include <Esp.h>           // ESP.getEfuseMac() — reads MAC from efuse without needing WiFi.begin()

static Preferences prefs;

// NVS namespace + keys
static const char *NS         = "legacytape";
static const char *KEY_TOKEN  = "tok";        // 32 hex chars
static const char *KEY_PAIRED = "paired";     // bool
static const char *KEY_SSID   = "ssid";       // primary wifi SSID
static const char *KEY_PW     = "pw";         // primary wifi password
static const char *KEY_SSID2  = "ssid2";      // secondary wifi SSID (optional)
static const char *KEY_PW2    = "pw2";        // secondary wifi password (optional)
static const char *KEY_ACCT   = "acct";       // user account id (UUID)

// In-memory cached strings (stable pointers for callers).
static char s_device_id[12]  = {0};   // "LT-XXXXXX" + null
static char s_token_hex[33]  = {0};   // 32 hex + null
static char s_qr_url[96]     = {0};   // "legacytape://pair?d=LT-XXXXXX&t=<32>"
static char s_ssid[33]       = {0};   // primary, 32 chars max per WiFi spec
static char s_pw[64]         = {0};   // primary, 63 chars max
static char s_ssid2[33]      = {0};   // secondary (optional, "" if unused)
static char s_pw2[64]        = {0};   // secondary
static char s_acct[64]       = {0};   // UUID + safety margin

static volatile bool s_complete_event = false;   // single-shot edge flag

static void hex_encode(const uint8_t *bytes, size_t n, char *out) {
    static const char H[] = "0123456789abcdef";
    for (size_t i = 0; i < n; i++) {
        out[i * 2]     = H[(bytes[i] >> 4) & 0xF];
        out[i * 2 + 1] = H[bytes[i] & 0xF];
    }
    out[n * 2] = 0;
}

static void compute_device_id(void) {
    // ESP.getEfuseMac() reads the 48-bit chip MAC from efuse — works at boot,
    // no WiFi.begin() required. Bytes are in reversed order: if real MAC is
    // AA:BB:CC:DD:EE:FF then getEfuseMac() returns 0xFFEEDDCCBBAA.
    // We want the last 3 bytes of the real MAC (DD, EE, FF) = shifts 24, 32, 40.
    uint64_t mac = ESP.getEfuseMac();
    snprintf(s_device_id, sizeof(s_device_id), "LT-%02X%02X%02X",
             (uint8_t)((mac >> 24) & 0xFF),
             (uint8_t)((mac >> 32) & 0xFF),
             (uint8_t)((mac >> 40) & 0xFF));
}

void pairing_begin(void) {
    compute_device_id();

    prefs.begin(NS, false);   // RW namespace

    // Token: load or generate fresh 16 random bytes → 32 hex chars
    String stored = prefs.getString(KEY_TOKEN, "");
    if (stored.length() == 32) {
        strncpy(s_token_hex, stored.c_str(), 32);
        s_token_hex[32] = 0;
        Serial.println("[pairing] loaded existing token from NVS");
    } else {
        uint8_t raw[16];
        // First boot runs this before WiFi/BT are started, when the hardware
        // RNG is only guaranteed to be random with an entropy source enabled
        // (ESP-IDF "Random Number Generation"). Enable the internal one around
        // the read; nothing else uses the RF subsystem or the ADC yet.
        bootloader_random_enable();
        esp_fill_random(raw, sizeof(raw));
        bootloader_random_disable();
        hex_encode(raw, sizeof(raw), s_token_hex);
        prefs.putString(KEY_TOKEN, s_token_hex);
        Serial.println("[pairing] generated new token, persisted to NVS");
    }

    // Build the QR URL once
    snprintf(s_qr_url, sizeof(s_qr_url),
             "legacytape://pair?d=%s&t=%s",
             s_device_id, s_token_hex);

    // Load stored credentials if already paired
    bool paired = prefs.getBool(KEY_PAIRED, false);
    if (paired) {
        String ssid  = prefs.getString(KEY_SSID, "");
        String pw    = prefs.getString(KEY_PW, "");
        String ssid2 = prefs.getString(KEY_SSID2, "");
        String pw2   = prefs.getString(KEY_PW2, "");
        String acct  = prefs.getString(KEY_ACCT, "");
        strncpy(s_ssid,  ssid.c_str(),  sizeof(s_ssid) - 1);
        strncpy(s_pw,    pw.c_str(),    sizeof(s_pw) - 1);
        strncpy(s_ssid2, ssid2.c_str(), sizeof(s_ssid2) - 1);
        strncpy(s_pw2,   pw2.c_str(),   sizeof(s_pw2) - 1);
        strncpy(s_acct,  acct.c_str(),  sizeof(s_acct) - 1);
    }

    Serial.printf("[pairing] device ID: %s\n", s_device_id);
    // The token is a credential (it authorizes BLE provisioning and backend
    // calls), so the log shows the URL with the token masked.
    Serial.printf("[pairing] QR URL:    legacytape://pair?d=%s&t=<hidden>\n", s_device_id);
    Serial.printf("[pairing] paired:    %s\n", paired ? "yes" : "no");

    prefs.end();
}

bool pairing_is_complete(void) {
    prefs.begin(NS, true);    // read-only
    bool p = prefs.getBool(KEY_PAIRED, false);
    prefs.end();
    return p;
}

void pairing_mark_complete(void) {
    prefs.begin(NS, false);
    prefs.putBool(KEY_PAIRED, true);
    prefs.end();
    s_complete_event = true;
    Serial.println("[pairing] marked complete in NVS");
}

bool pairing_consume_complete_event(void) {
    if (s_complete_event) {
        s_complete_event = false;
        return true;
    }
    return false;
}

bool pairing_store_credentials(const char *ssid,  const char *pw,
                               const char *ssid2, const char *pw2,
                               const char *acct) {
    if (!ssid || !pw || !acct) return false;
    if (strlen(ssid) > 32 || strlen(pw) > 63 || strlen(acct) > 63) return false;
    // ssid2/pw2 are optional; treat NULL as empty string
    const char *s2 = ssid2 ? ssid2 : "";
    const char *p2 = pw2   ? pw2   : "";
    if (strlen(s2) > 32 || strlen(p2) > 63) return false;

    strncpy(s_ssid,  ssid, sizeof(s_ssid) - 1);  s_ssid[sizeof(s_ssid) - 1] = 0;
    strncpy(s_pw,    pw,   sizeof(s_pw) - 1);    s_pw[sizeof(s_pw) - 1] = 0;
    strncpy(s_ssid2, s2,   sizeof(s_ssid2) - 1); s_ssid2[sizeof(s_ssid2) - 1] = 0;
    strncpy(s_pw2,   p2,   sizeof(s_pw2) - 1);   s_pw2[sizeof(s_pw2) - 1] = 0;
    strncpy(s_acct,  acct, sizeof(s_acct) - 1);  s_acct[sizeof(s_acct) - 1] = 0;

    prefs.begin(NS, false);
    bool ok = prefs.putString(KEY_SSID,  s_ssid)  > 0
           && prefs.putString(KEY_PW,    s_pw)    > 0
           && prefs.putString(KEY_SSID2, s_ssid2) >= 0   // allow empty
           && prefs.putString(KEY_PW2,   s_pw2)   >= 0
           && prefs.putString(KEY_ACCT,  s_acct)  > 0;
    prefs.end();
    if (ok) {
        Serial.printf("[pairing] credentials stored: ssid='%s' ssid2='%s' acct=%s\n",
                      s_ssid, s_ssid2, s_acct);
    }
    return ok;
}

const char *pairing_get_wifi_ssid(void)  { return s_ssid; }
const char *pairing_get_wifi_pw(void)    { return s_pw; }
const char *pairing_get_wifi_ssid2(void) { return s_ssid2; }
const char *pairing_get_wifi_pw2(void)   { return s_pw2; }
const char *pairing_get_account(void)    { return s_acct; }

void pairing_factory_reset(void) {
    prefs.begin(NS, false);
    prefs.clear();
    prefs.end();
    s_token_hex[0] = 0;
    s_qr_url[0] = 0;
    Serial.println("[pairing] factory reset — token + paired flag wiped");
}

const char *pairing_get_qr_url(void)     { return s_qr_url; }
const char *pairing_get_device_id(void)  { return s_device_id; }
const char *pairing_get_token_hex(void)  { return s_token_hex; }
