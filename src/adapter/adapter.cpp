/**
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009-2018 ObdDiag.Net. All rights reserved.
 *
 */

#include <lstring.h>
#include <cctype>
#include <LPC15xx.h>
#include <Timer.h>
#include <GpioDrv.h>
#include <CmdUart.h>
#include <CanDriver.h>
#include <EcuUart.h>
#include <PwmDriver.h>
#include <AdcDriver.h>
#include <led.h>
#include "adaptertypes.h"
#include "datacollector.h"


using namespace std;
using namespace util;

const int UART_SPEED = 115200;

static CmdUart* glblUart;
static DataCollector* collector;

/**
 * Enable the clocks and peripherals, initialize the drivers
 */
static void SetAllRegisters()
{
    // Enable MRT timer
    LPC_SYSCON->SYSAHBCLKCTRL1 |= (1 << 0);
    LPC_SYSCON->PRESETCTRL1 &= ~(1 << 0);
    
    // Enable STC2/SCT3 timers
    LPC_SYSCON->SYSAHBCLKCTRL1 |= (3 << 4);
    LPC_SYSCON->PRESETCTRL1 &= ~(3 << 4);

    // Enable RIT timer
    LPC_SYSCON->SYSAHBCLKCTRL1 |= (1 << 1);
    LPC_SYSCON->PRESETCTRL1 &= ~(1 << 1);
    
    LPC_SYSCON->SYSAHBCLKCTRL0 |= (1 << 11); // MUX
    LPC_SYSCON->SYSAHBCLKCTRL0 |= (1 << 12); // SVM
    LPC_SYSCON->SYSAHBCLKCTRL0 |= (1 << 13); // IOCON
    
    // LPC I/O pins
    LPC_SYSCON->SYSAHBCLKCTRL0 |= (1 << 14); // PIO0
    
    CmdUart::configure();
    EcuUart::configure();
    CanDriver::configure();
    AdptLED::configure();
    PwmDriver::configure();
    AdcDriver::configure();
}

/**
 * Outer interface UART receive callback
 * @param[in] ch Character received from UART
 */
static bool UserUartRcvHandler(uint8_t ch)
{
    bool ready = false;
    
    if (AdapterConfig::instance()->getBoolProperty(PAR_ECHO) && ch != '\n') {
        glblUart->send(ch);
        if (ch == '\r' && AdapterConfig::instance()->getBoolProperty(PAR_LINEFEED)) {
            glblUart->send('\n');
        }
    }
    
    if (ch == '\r') { // Got cmd terminator
        ready = true;
    }
    else if (isprint(ch)) { // this will skip '\n' as well
        collector->putChar(ch);
    }
    
    return ready;
}

/**
 * Send string to UART
 * @param[in] str String to send
 */
void AdptSendString(const util::string& str)
{
    glblUart->send(str);
}

/**
 * Adapter main loop
 */
static void AdapterRun() 
{
    collector = new DataCollector(RX_BUFFER_LEN, RX_RESERVED);
    Ecumsg::setReferenceData(collector->getData());
    
    glblUart = CmdUart::instance();
    glblUart->init(UART_SPEED);
    glblUart->handler(UserUartRcvHandler);
    AdptPowerModeConfigure();
    AdptDispatcherInit();
    
    for(;;) {    
        if (glblUart->ready()) {
            glblUart->ready(false);
            AdptOnCmd(collector);
            collector->reset();
        }
        else {
            AdptCheckHeartBeat();
        }
        //__WFI(); // goto sleep
    }
}

int main(void)
{
    SystemCoreClockUpdate();
    
    SetAllRegisters();
    AdapterRun();
}

