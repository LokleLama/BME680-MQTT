/* header files */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/i2c-dev.h>

#include <time.h>

#include "mosquitto.h"
#include "bme680.h"
#include "argEval.h"

struct config_t {
	int i2cFid; // I2C Linux device handle
	int i2c_address;
	int intervall;
	int single;
	int verbose;

	const char* csv_basename;
	const char* csv_ending;
	const char* csv_fullname;
	const char* device_name;

	const char* mqtt_address;
	int   mqtt_port;
	const char* mqtt_topic;

	FILE* comm_log;
} _config = {
	.i2c_address     = BME680_I2C_ADDR_PRIMARY,
	.intervall       = 10,
	.single          = 0,
	.verbose         = 0,

	.csv_basename    = "log",
	.csv_ending      = "csv",
	.csv_fullname    = "log.csv",
	.device_name     = "/dev/i2c-1",

	.mqtt_address    = NULL,
	.mqtt_port       = 1883,
	.mqtt_topic      = "location/measurement/bme680",

	.comm_log        = NULL
};

static int setCSVFileFunction(int argc, char** argv){
	static char* allocated_name = NULL;

	if(allocated_name != NULL){
		free(allocated_name);
	}

	int len = strlen(argv[0]);
	allocated_name = malloc(len +strlen(_config.csv_ending) +4);

	const char* point = strrchr(argv[0], '.');
	if(point == NULL){
		sprintf(allocated_name, "%s.%s", argv[0], _config.csv_ending);
		_config.csv_basename = argv[0];
		_config.csv_fullname = allocated_name;
	}else{
		int namelen = point -argv[0];
		memcpy(allocated_name, argv[0], namelen);
		allocated_name[namelen] = 0;
		_config.csv_fullname = argv[0];
		_config.csv_basename = allocated_name;
		_config.csv_ending = &point[1];
	}

	printf("Filename: full=%s, base=%s, end=%s\n", _config.csv_fullname, _config.csv_basename, _config.csv_ending);

    return 0;
}

static ArgumentDefinition_t _argArray[] = {
  {"h", "help"      , "shows this screen"                                                         , 0, vType_none     , NULL                  , NULL},
  {"a", "address"   , "enables the mqtt output by setting the server address (default: disabled)" , 1, vType_string   , &_config.mqtt_address , NULL},
  {"p", "port"      , "sets the mqtt server port (default: {})"                                   , 1, vType_integer  , &_config.mqtt_port    , NULL},
  {"t", "topic"     , "sets the default topic path (default: {})"                                 , 1, vType_string   , &_config.mqtt_topic   , NULL},
  {"i", "intervall" , "specifies the measurement intervall [s] (default: {}s)"                    , 1, vType_integer  , &_config.intervall    , NULL},
  {"o", "csvFile"   , "specifies the output file name base"                                       , 1, vType_callback , setCSVFileFunction    , NULL},
  {"s", "single"    , "only make a single measurement"                                            , 0, vType_none     , NULL                  , &_config.single},
  {"v", "verbose"   , "enables debug messages"                                                    , 0, vType_none     , NULL                  , &_config.verbose},
};

// open the Linux device
void i2cOpen()
{
	_config.i2cFid = open(_config.device_name, O_RDWR);
	if (_config.i2cFid < 0) {
		perror("i2cOpen");
		exit(1);
	}
#ifdef VERBOSE
	printf("Opened %s\n", _config.device_name);
#endif
}

// close the Linux device
void i2cClose()
{
#ifdef VERBOSE
	printf("Closed %s\n", _config.device_name);
#endif
	close(_config.i2cFid);
}

// set the I2C slave address for all subsequent I2C device transfers
void i2cSetAddress(int address)
{
	if (ioctl(_config.i2cFid, I2C_SLAVE, address) < 0) {
		perror("i2cSetAddress");
		exit(1);
	}
#ifdef VERBOSE
	printf("Set Address %02X\n", address);
#endif
}

/*
 * Write operation in either I2C or SPI
 *
 * param[in]        dev_addr        I2C or SPI device address
 * param[in]        reg_addr        register address
 * param[in]        reg_data_ptr    pointer to the data to be written
 * param[in]        data_len        number of bytes to be written
 *
 * return          result of the bus communication function
 */
static int8_t bus_write(uint8_t dev_id, uint8_t reg_addr, uint8_t *data, uint16_t data_len)
{
	int8_t rslt = 0; /* Return 0 for Success, non-zero for failure */

	uint8_t reg[16];
	reg[0] = reg_addr;

	if(_config.comm_log != NULL) fprintf(_config.comm_log, "W %02X ->", reg_addr);

	for (int i = 0; i < data_len; i++){
		if(_config.comm_log != NULL) fprintf(_config.comm_log, " %02X", data[i]);
		reg[i +1] = data[i];
	}
	if(_config.comm_log != NULL) fprintf(_config.comm_log, "\n");

	if (write(_config.i2cFid, reg, data_len+1) != data_len+1) {
		perror("user_i2c_write");
		rslt = 1;
		exit(1);
	}

	return rslt;
}

/*
 * Read operation in either I2C or SPI
 *
 * param[in]        dev_addr        I2C or SPI device address
 * param[in]        reg_addr        register address
 * param[out]       reg_data_ptr    pointer to the memory to be used to store
 *                                  the read data
 * param[in]        data_len        number of bytes to be read
 *
 * return          result of the bus communication function
 */
static int8_t bus_read(uint8_t dev_id, uint8_t reg_addr, uint8_t *data, uint16_t len)
{
	int8_t rslt = 0; /* Return 0 for Success, non-zero for failure */

	if(_config.comm_log != NULL) fprintf(_config.comm_log, "R %02X <-", reg_addr);

	if (write(_config.i2cFid, &reg_addr, 1) != 1) {
		perror("user_i2c_read_reg");
		rslt = 1;
	}

	if (read(_config.i2cFid, data, len) != len) {
		perror("user_i2c_read_data");
		rslt = 1;
	}

	for(int n = 0; n < len && _config.comm_log != NULL; n++){
		fprintf(_config.comm_log, " %02X", data[n]);
	}
	if(_config.comm_log != NULL) fprintf(_config.comm_log, "\n");

	return rslt;
}

/*
 * System specific implementation of sleep function
 *
 * param[in]       t_ms    time in milliseconds
 *
 * return          none
 */
static void _sleep(uint32_t t_ms)
{
	usleep(t_ms *1000);
}

/*
 * Capture the system time in microseconds
 *
 * return          system_current_time    system timestamp in microseconds
 */
int64_t get_timestamp_us()
{
	struct timespec spec;
	//clock_gettime(CLOCK_REALTIME, &spec);
	/* MONOTONIC in favor of REALTIME to avoid interference by time sync. */
	clock_gettime(CLOCK_MONOTONIC, &spec);

	int64_t system_current_time_ns = (int64_t)(spec.tv_sec) * (int64_t)1000000000
									+ (int64_t)(spec.tv_nsec);
	int64_t system_current_time_us = system_current_time_ns / 1000;

	return system_current_time_us;
}

static void HandleBMEError(int8_t err){
	if(err == BME680_OK) return;

#ifdef VERBOSE
	printf("Error occured in BME Sensor (%d)\n", err);
#endif
	fprintf(stderr, "Error from BME Sensor\n");
	exit(1);
}

static const char* GetCurrentTime(){
	static char buffer[40];
	time_t rawtime;
	struct tm* timeinfo;

	rawtime = time(NULL);
	timeinfo = localtime(&rawtime);

	strftime(buffer, 40, "%F, %T", timeinfo);
	return buffer;
}

static const char* GetNextCSVFileName(){
	static char buffer[256];
	static time_t buffertime = 0;
	time_t rawtime = time(NULL);

	if(rawtime == buffertime) return buffer;

	struct tm* timeinfo = localtime(&rawtime);

	char tmp[256];
	snprintf(tmp, sizeof(tmp), "%s_%%F_%%a.%s", _config.csv_basename, _config.csv_ending);

	strftime(buffer, sizeof(buffer), tmp, timeinfo);
	return buffer;
}

static void OutputCSV(struct bme680_field_data* data){
	static char old_filename[256] = "";

	if(_config.csv_fullname == NULL) return;

	const char* filemode = "a";
	if(strlen(old_filename) == 0){
		const char* next_filename = GetNextCSVFileName();
		//filemode = "w";
		strcpy(old_filename, next_filename);
	}else{
		const char* next_filename = GetNextCSVFileName();
		if(strcmp(next_filename, old_filename) != 0){
			char cmd[512];
			sprintf(cmd, "cp %s %s", _config.csv_fullname, old_filename);
			system(cmd);
			strcpy(old_filename, next_filename);

			filemode = "w";
		}
	}

	FILE* fd = fopen(_config.csv_fullname, filemode);
	if(fd == NULL){
		fprintf(stderr, "Unable to open %s!", _config.csv_fullname);
		_config.csv_fullname = NULL;
		return;
	}

	if(filemode[0] != 'a'){
		fprintf(fd, "Date, Time, Temperature [°C], Pressure [hPa], Humidity [%%rH], AirQualiry [Ohms]\n");
	}

	fprintf(fd, "%s, %.4f, %.6f, %.4f", GetCurrentTime(), data->temperature, data->pressure /100, data->humidity);
        /* Avoid using measurements from an unstable heating setup */
	if(data->status & BME680_GASM_VALID_MSK)
		fprintf(fd, ", %.2f", data->gas_resistance);
	fprintf(fd, "\n");

	fclose(fd);
}

const char* _json_templeate = "{ \
	\"Temperature\": %.4f, \
	\"Humidity\": %.4f, \
	\"Pressure\": %.2f, \
	\"Quality\": %.1f, \
	\"TemperatureUnit\": \"°C\", \
	\"HumidityUnit\": \"%%rh\", \
	\"PressureUnit\": \"hPa\", \
	\"QualityUnit\": \"Ohm\" \
}";

static void publishData(struct mosquitto *mosq, struct bme680_field_data* data){
	if(mosq == NULL) return;

	static char tmp[512];
	snprintf(tmp, sizeof(tmp), _json_templeate, data->temperature, data->humidity, data->pressure /100, data->gas_resistance);

	int err = mosquitto_publish(mosq, NULL, _config.mqtt_topic, strlen(tmp), tmp, 0 /*MQTT_QOS_0*/, false);
	if(err != MOSQ_ERR_SUCCESS){
		fprintf(stderr, "Error publishing: %s\n", mosquitto_strerror(err));
	}
}

int main(int argc, char** argv)
{
	//argEval_enableHelpWithoutArguments();
	//argEval_registerCallbackForExtraArguments(extraArgumentFunction);
	argEval_registerArguments(_argArray, sizeof (_argArray) / sizeof (ArgumentDefinition_t), &_argArray[0]);
	argEval_Parse(argc, argv, stderr);

	if(_config.intervall <= 0){
		_config.intervall = 1;
	}

	i2cOpen();
	i2cSetAddress(_config.i2c_address);

	struct bme680_dev bme_dev;
	memset(&bme_dev, 0, sizeof(bme_dev));
	bme_dev.intf = BME680_I2C_INTF;
	bme_dev.amb_temp = 25;

	bme_dev.read = bus_read;
	bme_dev.write = bus_write;
	bme_dev.delay_ms = _sleep;

	HandleBMEError(bme680_init(&bme_dev));


	bme_dev.tph_sett.os_hum = BME680_OS_2X;
    bme_dev.tph_sett.os_pres = BME680_OS_4X;
    bme_dev.tph_sett.os_temp = BME680_OS_8X;
    bme_dev.tph_sett.filter = BME680_FILTER_SIZE_3;

    /* Set the remaining gas sensor settings and link the heating profile */
    bme_dev.gas_sett.run_gas = BME680_ENABLE_GAS_MEAS;
    /* Create a ramp heat waveform in 3 steps */
    bme_dev.gas_sett.heatr_temp = 320; /* degree Celsius */
    bme_dev.gas_sett.heatr_dur = 150; /* milliseconds */

    /* Select the power mode */
    /* Must be set before writing the sensor configuration */
    bme_dev.power_mode = BME680_FORCED_MODE; 

    /* Set the required sensor settings needed */
    uint8_t set_required_settings = BME680_OST_SEL | BME680_OSP_SEL | BME680_OSH_SEL | BME680_FILTER_SEL | BME680_GAS_SENSOR_SEL;

    /* Set the desired sensor configuration */
    HandleBMEError(bme680_set_sensor_settings(set_required_settings, &bme_dev));

    /* Set the power mode */
    HandleBMEError(bme680_set_sensor_mode(&bme_dev));


	/* Get the total measurement duration so as to sleep or wait till the
     * measurement is complete */
    uint16_t meas_period;
    bme680_get_profile_dur(&meas_period, &bme_dev);
	
    _sleep(meas_period); /* Delay till the measurement is ready */

	if(meas_period < _config.intervall) meas_period = _config.intervall;
	printf("Measurement Periode will be %dms\n", meas_period);

	struct mosquitto *mosq = NULL;
	if(_config.mqtt_address != NULL){
		//libmosquitto initialization
		mosquitto_lib_init();

		//Create new libmosquitto client instance
		mosq = mosquitto_new(NULL, true, NULL);
		if (mosq == NULL){
			fprintf(stderr, "Error: failed to create mosquitto client\n");
		}else{
			//Connect to MQTT broker
			int err = mosquitto_connect(mosq, _config.mqtt_address, _config.mqtt_port, 60);
			if (err != MOSQ_ERR_SUCCESS)
			{
				fprintf(stderr, "Error: connecting to MQTT broker failed\n");
				fprintf(stderr, "Error: %s\n", mosquitto_strerror(err));
				mosquitto_destroy(mosq);
    			mosquitto_lib_cleanup();
				mosq = NULL;
			}
		}
	}

    struct bme680_field_data data;
	int count = 0;
    while(_config.single == 0 || count < _config.single)
    {
		HandleBMEError(bme680_get_sensor_data(&data, &bme_dev));
		OutputCSV(&data);
		publishData(mosq, &data);

        printf("T: %.2f degC, P: %.2f hPa, H %.2f %%rH ", data.temperature, data.pressure /100, data.humidity);
        /* Avoid using measurements from an unstable heating setup */
        if(data.status & BME680_GASM_VALID_MSK)
            printf(", G: %.0f ohms", data.gas_resistance);

        printf("\r\n");

        /* Trigger the next measurement if you would like to read data out continuously */
        if (bme_dev.power_mode == BME680_FORCED_MODE) {
            HandleBMEError(bme680_set_sensor_mode(&bme_dev));
        }
        _sleep(_config.intervall *1000);
		count++;
    }

	i2cClose();
	if(mosq != NULL){
		//Clean up/destroy objects created by libmosquitto
    	mosquitto_destroy(mosq);
    	mosquitto_lib_cleanup();
	}
	return 0;
}

