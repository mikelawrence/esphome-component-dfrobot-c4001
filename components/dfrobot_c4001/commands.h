#pragma once

#include <cmath>
#include <cstdint>

namespace esphome {
namespace dfrobot_c4001 {

static const uint8_t COMMAND_QUEUE_SIZE = 32;

// Enumeration for Command States
enum CommandState : uint8_t {
  STATE_CMD_SEND = 0,  // command needs to be sent
  STATE_WAIT_ECHO,     // command was sent, now waiting for command echo
  STATE_PROCESS,       // command in process
  STATE_WAIT_PROMPT,   // waiting for prompt, terminates this transaction
  STATE_DONE,
};

enum class CommandType : uint8_t {
  POWER,
  GET_RANGE,
  SET_RANGE,
  GET_TRIG_RANGE,
  SET_TRIG_RANGE,
  GET_SENSITIVITY,
  SET_SENSITIVITY,
  GET_LATENCY,
  SET_LATENCY,
  GET_INHIBIT_TIME,
  SET_INHIBIT_TIME,
  GET_THR_FACTOR,
  SET_THR_FACTOR,
  SET_UART_OUTPUT,
  SET_LED_MODE,
  SET_GPIO_MODE,
  GET_GPIO_MODE,
  GET_MICRO_MOTION,
  SET_MICRO_MOTION,
  FACTORY_RESET,
  RESET_SYSTEM,
  SAVE_CFG,
  SET_RUN_APP,
  GET_HWV,
  GET_SWV,
};

// A queued command, stored by value (no heap allocation).
// a/b hold the float arguments of set commands or the parsed values of get commands (NAN = not parsed).
// flag holds the on/off argument (power, LED, UART, GPIO, micro motion, run app, reset read_config).
struct Command {
  CommandType type{CommandType::POWER};
  uint8_t state{STATE_CMD_SEND};
  int8_t retries_left{2};
  int8_t error_count{0};
  bool done{false};
  bool error{false};
  bool retry_power_stop{false};
  bool flag{false};
  float a{NAN};
  float b{NAN};
};

// Fixed-size ring buffer of commands. push_back keeps one slot free so a sensorStop
// can always be pushed to the front when a command reports "sensor is not stopped".
class CommandQueue {
 public:
  bool push_back(const Command &cmd);
  bool push_front(const Command &cmd);
  void pop_front();
  Command &front() { return this->commands_[this->head_]; }
  bool empty() const { return this->count_ == 0; }

 protected:
  Command commands_[COMMAND_QUEUE_SIZE];
  uint8_t head_{0};
  uint8_t count_{0};
};

}  // namespace dfrobot_c4001
}  // namespace esphome
