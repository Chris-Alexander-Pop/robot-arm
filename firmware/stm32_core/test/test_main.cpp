extern void run_packet_codec_tests();
extern void run_packet_state_encoding_tests();
extern void run_pid_controller_tests();

int main() {
  run_packet_codec_tests();
  run_packet_state_encoding_tests();
  run_pid_controller_tests();
  return 0;
}