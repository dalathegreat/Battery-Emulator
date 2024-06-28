
void init_serial();
void init_stored_settings();
void init_CAN();
void init_contactors();
void init_rs485();
void init_inverter();
void init_battery();
void receive_can();
void send_can();
void send_can2();
void update_SOC();
void update_values_inverter();
void init_serialDataLink();
void storeSettings();
void check_reset_reason();
void connectivity_loop(void* task_time_us);
void core_loop(void* task_time_us);
void init_mDNS();

