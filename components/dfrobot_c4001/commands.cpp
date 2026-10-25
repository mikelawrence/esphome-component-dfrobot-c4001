#include "dfrobot_c4001.h"
#include "commands.h"

#include "esphome/core/log.h"
#include "esphome/core/version.h"

namespace esphome {
namespace dfrobot_c4001 {
#if ESPHOME_VERSION_CODE >= VERSION_CODE(2026, 10, 0)
ESPHOME_LOG_TAG(TAG, "dfrobot_c4001.commands");
#else
static const char *const TAG = "dfrobot_c4001.commands";
#endif

static constexpr uint32_t CMD_TIMEOUT_MS = 1500;

static const char *safe_token(const char *token) { return token != nullptr ? token : ""; }

static float parse_float(const char *token) {
  if (token == nullptr)
    return NAN;
  auto value = parse_number<float>(token);
  return value.has_value() ? value.value() : NAN;
}

// Malformed reply: leave the command pending so the timeout/resend logic handles it
static void parse_failed(Command &cmd) {
  ESP_LOGD(TAG, "Failed to parse response");
  cmd.error = true;
}

bool CommandQueue::push_back(const Command &cmd) {
  if (this->count_ >= COMMAND_QUEUE_SIZE - 1)
    return false;
  this->commands_[(this->head_ + this->count_) % COMMAND_QUEUE_SIZE] = cmd;
  this->count_++;
  return true;
}

bool CommandQueue::push_front(const Command &cmd) {
  if (this->count_ >= COMMAND_QUEUE_SIZE)
    return false;
  this->head_ = (this->head_ + COMMAND_QUEUE_SIZE - 1) % COMMAND_QUEUE_SIZE;
  this->commands_[this->head_] = cmd;
  this->count_++;
  return true;
}

void CommandQueue::pop_front() {
  if (this->count_ == 0)
    return;
  this->head_ = (this->head_ + 1) % COMMAND_QUEUE_SIZE;
  this->count_--;
}

void DFRobotC4001Hub::format_command_(const Command &cmd, char *buf, size_t size) const {
  switch (cmd.type) {
    case CommandType::COMMAND_TYPE_POWER:
      snprintf(buf, size, "%s", cmd.flag ? "sensorStart" : "sensorStop");
      break;
    case CommandType::COMMAND_TYPE_GET_RANGE:
      snprintf(buf, size, "getRange");
      break;
    case CommandType::COMMAND_TYPE_SET_RANGE:
      snprintf(buf, size, "setRange %.3f %.3f", cmd.a, cmd.b);
      break;
    case CommandType::COMMAND_TYPE_GET_TRIG_RANGE:
      snprintf(buf, size, "getTrigRange");
      break;
    case CommandType::COMMAND_TYPE_SET_TRIG_RANGE:
      snprintf(buf, size, "setTrigRange %.3f", cmd.a);
      break;
    case CommandType::COMMAND_TYPE_GET_SENSITIVITY:
      snprintf(buf, size, "getSensitivity");
      break;
    case CommandType::COMMAND_TYPE_SET_SENSITIVITY:
      snprintf(buf, size, "setSensitivity %.0f %.0f", roundf(cmd.a), roundf(cmd.b));
      break;
    case CommandType::COMMAND_TYPE_GET_LATENCY:
      snprintf(buf, size, "getLatency");
      break;
    case CommandType::COMMAND_TYPE_SET_LATENCY:
      snprintf(buf, size, "setLatency %.3f %.3f", cmd.a, cmd.b);
      break;
    case CommandType::COMMAND_TYPE_GET_INHIBIT_TIME:
      snprintf(buf, size, "getInhibit");
      break;
    case CommandType::COMMAND_TYPE_SET_INHIBIT_TIME:
      snprintf(buf, size, "setInhibit %.3f", cmd.a);
      break;
    case CommandType::COMMAND_TYPE_GET_THR_FACTOR:
      snprintf(buf, size, "getThrFactor");
      break;
    case CommandType::COMMAND_TYPE_SET_THR_FACTOR:
      snprintf(buf, size, "setThrFactor %.3f", cmd.a);
      break;
    case CommandType::COMMAND_TYPE_SET_UART_OUTPUT:
      snprintf(buf, size, "%s", cmd.flag ? "setUartOutput 1 1 1 1.0" : "setUartOutput 1 0 1 1.0");
      break;
    case CommandType::COMMAND_TYPE_SET_LED_MODE:
      snprintf(buf, size, "setLedMode 1 %d", cmd.flag ? 0 : 1);
      break;
    case CommandType::COMMAND_TYPE_SET_GPIO_MODE:
      snprintf(buf, size, "setGpioMode 1 %d", cmd.flag ? 1 : 2);
      break;
    case CommandType::COMMAND_TYPE_GET_GPIO_MODE:
      snprintf(buf, size, "getGpioMode 1");
      break;
    case CommandType::COMMAND_TYPE_GET_MICRO_MOTION:
      snprintf(buf, size, "getMicroMotion");
      break;
    case CommandType::COMMAND_TYPE_SET_MICRO_MOTION:
      snprintf(buf, size, "setMicroMotion %d", cmd.flag ? 1 : 0);
      break;
    case CommandType::COMMAND_TYPE_FACTORY_RESET:
      snprintf(buf, size, "resetCfg");
      break;
    case CommandType::COMMAND_TYPE_RESET_SYSTEM:
      snprintf(buf, size, "resetSystem");
      break;
    case CommandType::COMMAND_TYPE_SAVE_CFG:
      snprintf(buf, size, "saveConfig");
      break;
    case CommandType::COMMAND_TYPE_SET_RUN_APP:
      snprintf(buf, size, "setRunApp %d", cmd.flag ? 1 : 0);
      break;
    case CommandType::COMMAND_TYPE_GET_HWV:
      snprintf(buf, size, "getHWV");
      break;
    case CommandType::COMMAND_TYPE_GET_SWV:
      snprintf(buf, size, "getSWV");
      break;
    default:
      buf[0] = '\0';
      break;
  }
}

// returns
//  negative number: failed, abs(return value) is the number of errors that occurred
//  1: success
//  0: not done yet
// if cmd.retry_power_stop = true then command reported "sensor not stopped", need to send sensorStop command
int8_t DFRobotC4001Hub::execute_(Command &cmd) {
  auto result = [&cmd]() -> int8_t { return cmd.error_count < 0 ? cmd.error_count : 1; };
  auto finish = [&cmd, &result]() -> int8_t {
    cmd.state = STATE_DONE;
    return result();
  };

  // send command is separate from the rest of the state machine
  if (cmd.state == STATE_CMD_SEND) {
    cmd.done = false;
    cmd.error = false;
    cmd.retry_power_stop = false;
    this->format_command_(cmd, this->cmd_buf_, sizeof(this->cmd_buf_));
    this->send_cmd_(this->cmd_buf_);
    cmd.state = STATE_WAIT_ECHO;
    return 0;
  }
  if (cmd.state == STATE_DONE)
    return result();

  // surround state machine with a successful read message
  if (this->read_message_()) {
    switch (cmd.state) {
      case STATE_WAIT_ECHO:
        if (cmd.type == CommandType::COMMAND_TYPE_RESET_SYSTEM) {
          // resetSystem command might return garbage, accept anything
          ESP_LOGV(TAG, "Send Cmd: Complete: %s", this->cmd_buf_);
          return finish();
        }
        if (strstr(this->read_buffer_, this->cmd_buf_) != nullptr) {
          cmd.state = STATE_PROCESS;
        }
        break;
      case STATE_PROCESS:
        if (strstr(this->read_buffer_, "Error") != nullptr) {
          cmd.error = true;
          cmd.state = STATE_WAIT_PROMPT;
          break;
        }
        if (strstr(this->read_buffer_, "sensor is not stopped") != nullptr) {
          // Let queue know we need to stop the sensor and then retry this command
          cmd.retry_power_stop = true;
          break;
        }
        this->on_message_(cmd);
        if (cmd.done) {
          cmd.state = STATE_WAIT_PROMPT;
        }
        break;
      case STATE_WAIT_PROMPT:
        if (strstr(this->read_buffer_, "DFRobot:/>") == nullptr)
          break;
        if (cmd.error || cmd.retry_power_stop) {
          if (cmd.retries_left > 0) {
            cmd.retries_left--;
            cmd.error_count--;
            // allow command to be used again
            cmd.state = STATE_CMD_SEND;
            if (cmd.retry_power_stop) {
              ESP_LOGD(TAG, "Command Retry: Sensor not stopped");
              // this command stays queued, but the queue needs to send a sensorStop first
              return result();
            }
            ESP_LOGD(TAG, "Command Retry");
            return 0;  // command not done
          }
          ESP_LOGE(TAG, "Command Failure");
          cmd.error_count--;
        } else {
          ESP_LOGV(TAG, "Send Cmd: Complete: %s", this->cmd_buf_);
        }
        return finish();
      default:
        return finish();
    }
  }

  // check for timeout
  if (App.get_loop_component_start_time() - this->ts_last_cmd_sent_ > CMD_TIMEOUT_MS) {
    if (cmd.type == CommandType::COMMAND_TYPE_RESET_SYSTEM) {
      // resetSystem command doesn't necessarily return anything, bypass retries
      ESP_LOGV(TAG, "Send Cmd: Reset System Command Timeout Bypassed");
      return finish();
    }
    if (cmd.retries_left > 0) {
      cmd.retries_left--;
      ESP_LOGD(TAG, "Command timeout: Retrying");
      cmd.state = STATE_CMD_SEND;
      cmd.error_count--;
      return 0;  // command not done
    }
    ESP_LOGE(TAG, "Command Failure: %s", this->cmd_buf_);
    cmd.error_count--;
    return finish();
  }
  return 0;
}

// Handles "Response v1 [v2]" and "Done" lines, skipping 'skip' tokens after "Response".
// Returns true on "Done".
bool DFRobotC4001Hub::parse_response_(Command &cmd, uint8_t skip, uint8_t count) {
  char *token = strtok(this->read_buffer_, " ");
  if (token == nullptr)
    return false;
  if (strcmp(token, "Done") == 0)
    return true;
  if (strcmp(token, "Response") == 0) {
    for (uint8_t i = 0; i < skip; i++)
      strtok(nullptr, " ");
    cmd.a = parse_float(strtok(nullptr, " "));
    if (count > 1)
      cmd.b = parse_float(strtok(nullptr, " "));
  }
  return false;
}

void DFRobotC4001Hub::on_message_(Command &cmd) {
  switch (cmd.type) {
    case CommandType::COMMAND_TYPE_GET_RANGE:
      if (this->parse_response_(cmd, 0, 2)) {
        if (std::isnan(cmd.a) || std::isnan(cmd.b)) {
          parse_failed(cmd);
        } else {
          this->set_min_range(cmd.a, false);
          this->set_max_range(cmd.b, false);
          cmd.done = true;
        }
      }
      break;
    case CommandType::COMMAND_TYPE_GET_TRIG_RANGE:
      if (this->parse_response_(cmd, 0, 1)) {
        if (std::isnan(cmd.a)) {
          parse_failed(cmd);
        } else {
          this->set_trigger_range(cmd.a, false);
          cmd.done = true;
        }
      }
      break;
    case CommandType::COMMAND_TYPE_GET_SENSITIVITY:
      if (this->parse_response_(cmd, 0, 2)) {
        if (std::isnan(cmd.a) || std::isnan(cmd.b)) {
          parse_failed(cmd);
        } else {
          this->set_hold_sensitivity(cmd.a, false);
          this->set_trigger_sensitivity(cmd.b, false);
          cmd.done = true;
        }
      }
      break;
    case CommandType::COMMAND_TYPE_GET_LATENCY:
      if (this->parse_response_(cmd, 0, 2)) {
        if (std::isnan(cmd.a) || std::isnan(cmd.b)) {
          parse_failed(cmd);
        } else {
          this->set_on_latency(cmd.a, false);
          this->set_off_latency(cmd.b, false);
          cmd.done = true;
        }
      }
      break;
    case CommandType::COMMAND_TYPE_GET_INHIBIT_TIME:
      if (this->parse_response_(cmd, 0, 1)) {
        if (std::isnan(cmd.a)) {
          parse_failed(cmd);
        } else {
          // The module tends to add 0.001 when this value is set, remove it here
          cmd.a = floorf(cmd.a * 500.0f) / 500.0f;
          this->set_inhibit_time(cmd.a, false);
          cmd.done = true;
        }
      }
      break;
    case CommandType::COMMAND_TYPE_GET_THR_FACTOR:
      if (this->parse_response_(cmd, 0, 1)) {
        if (std::isnan(cmd.a)) {
          parse_failed(cmd);
        } else {
          this->set_threshold_factor(cmd.a, false);
          cmd.done = true;
        }
      }
      break;
    case CommandType::COMMAND_TYPE_GET_GPIO_MODE:
      // "Response 1 <mode>", first value is always '1'
      if (this->parse_response_(cmd, 1, 1)) {
        if (std::isnan(cmd.a)) {
          parse_failed(cmd);
        } else {
          this->set_out_led_enable(cmd.a == 1.0f, false);
          cmd.done = true;
        }
      }
      break;
    case CommandType::COMMAND_TYPE_GET_MICRO_MOTION:
      if (this->parse_response_(cmd, 0, 1)) {
        if (std::isnan(cmd.a)) {
          parse_failed(cmd);
        } else {
          this->set_micro_motion_enable(cmd.a != 0.0f, false);
          cmd.done = true;
        }
      }
      break;
    case CommandType::COMMAND_TYPE_SET_MICRO_MOTION:
      if (strcmp(this->read_buffer_, "Done") == 0) {
        this->set_micro_motion_enable(cmd.flag, false);
        cmd.done = true;
      }
      break;
    case CommandType::COMMAND_TYPE_FACTORY_RESET:
      if (strstr(this->read_buffer_, "Done") != nullptr) {
        // reload settings
        this->set_run_led_enable(true);
        this->flash_run_led_enable();
        this->setup_module();
        this->config_load();
        cmd.done = true;
      }
      break;
    case CommandType::COMMAND_TYPE_RESET_SYSTEM:
      // this command responds with nothing, not even a command echo
      if (cmd.flag) {
        this->config_load();
      }
      ESP_LOGV(TAG, "Send Cmd: %s received something", this->cmd_buf_);
      cmd.done = true;
      break;
    case CommandType::COMMAND_TYPE_SAVE_CFG:
      if (strstr(this->read_buffer_, "no parameter has changed") != nullptr) {
        ESP_LOGV(TAG, "Send Cmd: Nothing Changed");
      } else if (strstr(this->read_buffer_, "Done") != nullptr) {
        this->config_load();
        cmd.done = true;
      }
      break;
    case CommandType::COMMAND_TYPE_GET_SWV:
    case CommandType::COMMAND_TYPE_GET_HWV: {
      const bool swv = cmd.type == CommandType::COMMAND_TYPE_GET_SWV;
      char *token = strtok(this->read_buffer_, ":");
      if (token == nullptr)
        break;
      if (strcmp(token, "Done") == 0) {
        cmd.done = true;
      } else if (strcmp(token, swv ? "SoftwareVersion" : "HardwareVersion") == 0) {
        token = strtok(nullptr, ":");
        if (token == nullptr) {
          parse_failed(cmd);
        } else if (swv) {
          this->set_software_version(token);
        } else {
          this->set_hardware_version(token);
        }
      }
      break;
    }
    default:
      if (strstr(this->read_buffer_, "Done") != nullptr)
        cmd.done = true;
      break;
  }
}

// Called from loop() when the command queue is empty
void DFRobotC4001Hub::read_state_() {
  if (!this->read_message_())
    return;
  if (strstr(this->read_buffer_, "$DFHPD,0, , , *") != nullptr) {
    this->set_occupancy(false);
    ESP_LOGV(TAG, "Recv Rpt: Occupancy Clear");
  } else if (strstr(this->read_buffer_, "$DFHPD,1, , , *") != nullptr) {
    this->set_occupancy(true);
    ESP_LOGV(TAG, "Recv Rpt: Occupancy Detected");
  } else if (strstr(this->read_buffer_, "$DFDMD") != nullptr) {
    strtok(this->read_buffer_, ",");
    auto target = parse_number<uint8_t>(safe_token(strtok(nullptr, ",")));
    strtok(nullptr, ",");
    auto target_distance = parse_number<float>(safe_token(strtok(nullptr, ",")));
    auto target_speed = parse_number<float>(safe_token(strtok(nullptr, ",")));
    auto target_energy = parse_number<float>(safe_token(strtok(nullptr, ",")));
    if (target.has_value() && target.value() == 1) {
      // one target is detected, that is all this sensor can do
      if (target_distance.has_value() && target_speed.has_value() && target_energy.has_value()) {
        this->set_target_distance(target_distance.value());
        this->set_target_speed(target_speed.value());
        this->set_target_energy(target_energy.value());
        this->set_occupancy(true);
        ESP_LOGV(TAG, "Recv Rpt: Target Detected, Dist=%.3f, Speed=%.3f, Energy=%u", target_distance.value(),
                 target_speed.value(), static_cast<unsigned int>(target_energy.value()));
      } else {
        ESP_LOGD(TAG, "Error parsing Distance and Speed Detection Output");
      }
    } else if (target.has_value() && target.value() == 0) {
      // no target is detected
      this->set_target_distance(0.0);
      this->set_target_speed(0.0);
      this->set_target_energy(0.0);
      this->set_occupancy(false);
      ESP_LOGV(TAG, "Recv Rpt: No Target");
    }
  }
}

}  // namespace esphome::dfrobot_c4001
