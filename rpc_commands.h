#ifndef RPCCOMMANDS_H
#define RPCCOMMANDS_H

//Códigos de los comandos y definicion de parametros para el protocolo RPC

// El estudiante debe añadir aqui cada nuevo comando que implemente. IMPORTANTE el orden de los comandos
// debe SER EL MISMO aqui, y en el codigo equivalente en la parte del microcontrolador (Code Composer)
typedef enum {
    COMMAND_REJECTED,
    COMMAND_PING,
    COMMAND_LED_GPIO,
    COMMAND_LED_PWM_BRIGHTNESS,
    COMMAND_LED_PWM_COLOR,
    COMMAND_BUTTONS_STATUS,
    COMMAND_BUTTONS_REQUEST,
    COMMAND_BUTTONS_ANSWER,
    COMMAND_SAMPLING_CONFIG,
    COMMAND_ADC_8BITS_SAMPLES,
    COMMAND_ADC_12BITS_SAMPLES,
    COMMAND_GSENSOR_COLOR_REQUEST,
    COMMAND_GSENSOR_COLOR_ANSWER,
    COMMAND_GSENSOR_GESTURE,
    COMMAND_GSENSOR_CONFIG_THRESHOLD,
    COMMAND_GSENSOR_THRESHOLD_EXCEED,
    COMMAND_GSENSOR_MODE_CONFIG,
    COMMAND_GSENSOR_FIFO
    //etc, etc...
} commandTypes;

//Estructuras relacionadas con los parametros de los comandos. El estuadiante debera crear las
// estructuras adecuadas a los comandos usados, y asegurarse de su compatibilidad en ambos extremos

#pragma pack(1)	//Cambia el alineamiento de datos en memoria a 1 byte.

typedef struct {
    uint8_t command;
} PARAMETERS_COMMAND_REJECTED;

typedef union{
    struct {
         uint8_t padding:1;
         uint8_t red:1;        
         uint8_t blue:1;
         uint8_t green:1;
    } leds;
    uint8_t value;
} PARAMETERS_LED_GPIO;

typedef struct {
    float rIntensity;
} PARAMETERS_LED_PWM_BRIGHTNESS;

typedef struct{
   uint32_t colors[3];
} PARAMETERS_LED_PWM_COLOR;

typedef union{
    struct {
        uint8_t button1:1;
        uint8_t padding:3;
        uint8_t button2:1;
    } buttons;
} PARAMETERS_BUTTONS_STATUS;

typedef union{
    struct {
        uint8_t button1:1;
        uint8_t padding:3;
        uint8_t button2:1;
    } buttons;
} PARAMETERS_BUTTONS_ANSWER;

typedef union{
    struct {
        uint8_t active:1;
        uint8_t mode12:1;
        uint16_t rate:14;
    } config;
} PARAMETERS_SAMPLING_CONFIG;

typedef struct{
    struct{
        uint8_t sample[8];
    } channel[4];
} PARAMETERS_ADC_8BITS_SAMPLES;

typedef struct{
    struct{
        uint16_t sample[8];
    } channel[4];
} PARAMETERS_ADC_12BITS_SAMPLES;

typedef struct
{
   uint16_t red;
   uint16_t green;
   uint16_t blue;
   uint16_t intensity;
} PARAMETERS_GSENSOR_COLOR_ANSWER;

typedef struct
{
    uint8_t gesture;
} PARAMETERS_GSENSOR_GESTURE;

typedef struct
{
    uint8_t threshold;
} PARAMETERS_GSENSOR_CONFIG_THRESHOLD;

typedef struct
{
    uint8_t size;
    uint8_t fifo[128];
} PARAMETERS_GSENSOR_FIFO;

typedef struct
{
    bool fifo_mode;
} PARAMETERS_GSENSOR_MODE_CONFIG;

#pragma pack()	//...Pero solo para los comandos que voy a intercambiar, no para el resto.

#endif // RPCCOMMANDS_H
