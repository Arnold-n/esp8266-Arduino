/*
AVR In-System Programming over WiFi for ESP8266
Copyright (c) Kiril Zyapkov <kiril@robotev.com>

Original version:
    ArduinoISP version 04m3
    Copyright (c) 2008-2011 Randall Bohn
    If you require a license, see
        http://www.opensource.org/licenses/bsd-license.php

Modified to include bit-banging functionality via selectable non-HSPI pins
Changes copyright (c) 2022 Arnold Niessen
To use non-HSPI, set spi_freq to 0 and define BB_*_PINs below or select pins via constructor
*/

#ifndef _ESP8266AVRISP_H
#define _ESP8266AVRISP_H

#include <Arduino.h>

// uncomment if you use an n-mos to level-shift the reset line
// #define AVRISP_ACTIVE_HIGH_RESET

// SPI clock frequency in Hz
// to select bit banging, define AVRISP_SPI_FREQ 0 here or set spi_freq to 0 via constructor
#define AVRISP_SPI_FREQ   300e3

// pin assignments for non-HSPI bit-banging
#define BB_CLK_PIN 15 // (GPIO15 usually has a pull-down resistor)
#define BB_MOSI_PIN 2 // (GPIO2 usually has a pull-up resistor)
#define BB_MISO_PIN 0 // (GPIO0 usually has a pull-up resistor)

// programmer states
typedef enum {
    AVRISP_STATE_IDLE = 0,    // no active TCP session
    AVRISP_STATE_PENDING,     // TCP connected, pending SPI activation
    AVRISP_STATE_ACTIVE       // programmer is active and owns the SPI bus
} AVRISPState_t;

// stk500 parameters
typedef struct {
    uint8_t devicecode;
    uint8_t revision;
    uint8_t progtype;
    uint8_t parmode;
    uint8_t polling;
    uint8_t selftimed;
    uint8_t lockbytes;
    uint8_t fusebytes;
    int flashpoll;
    int eeprompoll;
    int pagesize;
    int eepromsize;
    int flashsize;
} AVRISP_parameter_t;


class ESP8266AVRISP {
public:
    ESP8266AVRISP(uint16_t port, uint8_t reset_pin, uint32_t spi_freq=AVRISP_SPI_FREQ, bool reset_state=false, bool reset_activehigh=false, uint8_t clk_pin=BB_CLK_PIN, uint8_t mosi_pin=BB_MOSI_PIN, uint8_t miso_pin=BB_MISO_PIN);

    void begin();

    // set the SPI clock frequency
    void setSpiFrequency(uint32_t);

    // control the state of the RESET pin of the target
    // see AVRISP_ACTIVE_HIGH_RESET
    void setReset(bool);

    // check for pending clients if IDLE, check for disconnect otherwise
    // returns the updated state
    AVRISPState_t update();

    // transition to ACTIVE if PENDING
    // serve STK500 commands from buffer if ACTIVE
    // returns the updated state
    AVRISPState_t serve();

protected:
    uint8_t transfer(uint8_t a); // for non-HSPI bit banging transfer

    inline void _reject_incoming(void);     // reject any incoming tcp connections

    void avrisp(void);           // handle incoming STK500 commands

    uint8_t getch(void);        // retrieve a character from the remote end
    uint8_t spi_transaction(uint8_t, uint8_t, uint8_t, uint8_t);
    void empty_reply(void);
    void breply(uint8_t);

    void get_parameter(uint8_t);
    void set_parameters(void);
    int addr_page(int);
    void flash(uint8_t, int, uint8_t);
    void write_flash(int);
    uint8_t write_flash_pages(int length);
    uint8_t write_eeprom(int length);
    uint8_t write_eeprom_chunk(int start, int length);
    void commit(int addr);
    void program_page();
    uint8_t flash_read(uint8_t hilo, int addr);
    bool flash_read_page(int length);
    bool eeprom_read_page(int length);
    void read_page();
    void read_signature();

    void universal(void);

    void fill(int);             // fill the buffer with n bytes
    void start_pmode(void);     // enter program mode
    void end_pmode(void);       // exit program mode

    inline bool _resetLevel(bool reset_state) { return reset_state == _reset_activehigh; }

    uint32_t _spi_freq;
    WiFiServer _server;
    WiFiClient _client;
    AVRISPState_t _state;
    uint8_t _reset_pin;
    uint8_t _clk_pin;
    uint8_t _mosi_pin;
    uint8_t _miso_pin;
    bool _reset_state;
    bool _reset_activehigh;
    bool _use_hspi;

    // programmer settings, set by remote end
    AVRISP_parameter_t param;
    // page buffer
    uint8_t buff[256];

    int error = 0;
    bool pmode = 0;

    // address for reading and writing, set by 'U' command
    int here;
};


#endif // _ESP8266AVRISP_H
