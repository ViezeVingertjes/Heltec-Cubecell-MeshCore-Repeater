#include <Arduino.h>
extern "C" {
    void unityOutputStart(unsigned long) {}
    void unityOutputChar(unsigned int c) { Serial.write(static_cast<char>(c)); }
    void unityOutputFlush() { Serial.flush(); }
    void unityOutputComplete() {}
}
#include <unity.h>
void setUp() {}
void tearDown() {}
void test_buffer_starts_empty();
void test_push_and_pop();
void test_fifo_order();
void test_buffer_overflow_protection();
void test_buffer_underflow_protection();
void test_result_with_value();
void test_result_with_error();
void test_status_ok();
void test_status_error();
void test_version_defined();
void test_version_comparison();
void test_ed25519_creates_keypair();
void test_ed25519_same_seed_same_keys();
void test_ed25519_derives_public_key();
void test_flash_storage_init();
void test_flash_storage_write_read();
void test_flash_storage_single_byte();
void test_flash_storage_persists_after_commit();
void test_flash_storage_bounds_check();
void test_identity_generates_valid();
void test_identity_persists_across_loads();
void test_identity_hash_id_is_first_pubkey_byte();
void test_radio_init_succeeds();
void test_radio_start_receive_succeeds();
void test_radio_can_set_event_handler();
void test_radio_standby();
void test_radio_sleep();
void test_packet_construct_and_decode();
void test_packet_encode_decode_roundtrip();
void test_packet_with_transport_codes();
void test_packet_decode_rejects_invalid_input();
void test_packet_header_parsing();
void test_packet_payload_types();
void test_time_sync_consensus_basic();
void test_time_sync_power_loss_recovery();
void test_time_sync_extract_advert();
void test_time_sync_reset();
void test_time_sync_eviction();
void setup() {
    Serial.begin(115200);
    delay(2000);
    UNITY_BEGIN();
        RUN_TEST(test_buffer_starts_empty);
    RUN_TEST(test_push_and_pop);
    RUN_TEST(test_fifo_order);
    RUN_TEST(test_buffer_overflow_protection);
    RUN_TEST(test_buffer_underflow_protection);
        RUN_TEST(test_result_with_value);
    RUN_TEST(test_result_with_error);
    RUN_TEST(test_status_ok);
    RUN_TEST(test_status_error);
        RUN_TEST(test_version_defined);
    RUN_TEST(test_version_comparison);
        RUN_TEST(test_ed25519_creates_keypair);
    RUN_TEST(test_ed25519_same_seed_same_keys);
    RUN_TEST(test_ed25519_derives_public_key);
        RUN_TEST(test_flash_storage_init);
    RUN_TEST(test_flash_storage_write_read);
    RUN_TEST(test_flash_storage_single_byte);
    RUN_TEST(test_flash_storage_persists_after_commit);
    RUN_TEST(test_flash_storage_bounds_check);
        RUN_TEST(test_identity_generates_valid);
    RUN_TEST(test_identity_persists_across_loads);
    RUN_TEST(test_identity_hash_id_is_first_pubkey_byte);
        RUN_TEST(test_radio_init_succeeds);
    RUN_TEST(test_radio_start_receive_succeeds);
    RUN_TEST(test_radio_can_set_event_handler);
    RUN_TEST(test_radio_standby);
    RUN_TEST(test_radio_sleep);
        RUN_TEST(test_packet_construct_and_decode);
    RUN_TEST(test_packet_encode_decode_roundtrip);
    RUN_TEST(test_packet_with_transport_codes);
    RUN_TEST(test_packet_decode_rejects_invalid_input);
    RUN_TEST(test_packet_header_parsing);
    RUN_TEST(test_packet_payload_types);
        RUN_TEST(test_time_sync_consensus_basic);
    RUN_TEST(test_time_sync_power_loss_recovery);
    RUN_TEST(test_time_sync_extract_advert);
    RUN_TEST(test_time_sync_reset);
    RUN_TEST(test_time_sync_eviction);
        UNITY_END();
}
void loop() {
    delay(100);
}
