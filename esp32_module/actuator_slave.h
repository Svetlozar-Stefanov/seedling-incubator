
typedef enum {
  READ_SOIL_MOISTURE
} command;

typedef enum {
  ACTUATOR_SUCCESS = 0,
  ACTUATOR_ERR_INITIALIZATION_FAIL,
  ACTUATOR_ERR_FAILED_TO_CONNECT_TO_MASTER,
  ACTUATOR_ERR_FAILED_TO_READ_MAC_ADDRESS
} actuator_err_t;
const char* getActuatorErrorMessage(actuator_err_t err);

typedef enum {
  COMMAND_SUCCESS = 0,
  COMMAND_ERR_SEND_UNSUCCESSFUL
} command_err_t;
const char* getCommandErrorMessage(command_err_t err);

actuator_err_t printMacAddress();

actuator_err_t setupActuatorSlave();