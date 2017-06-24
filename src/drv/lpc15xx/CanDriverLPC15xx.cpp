/**
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009-2016 ObdDiag.Net. All rights reserved.
 *
 */

#include <cstring>
#include <cstdlib>
#include <adaptertypes.h>
#include <LPC15xx.h>
#include <romapi_15xx.h>
#include "CanDriver.h"
#include "GPIODrv.h"
#include <canmsgbuffer.h>
#include <led.h>

using namespace std;

const int TxPin = 18;
const int RxPin =  9;
const int RxPort = 0;
const int TxPort = 0;
const uint32_t PinAssign = ((RxPin << 16) + (RxPort * 32)) | ((TxPin << 8)  + (TxPort * 32));
const uint32_t CAN_MSGOBJ_STD = 0x00000000;
const uint32_t CAN_MSGOBJ_EXT = 0x20000000;
const int FIFO_NUM = 10;

// Driver static variables
CAN_HANDLE_T CanDriver::handle_;
static volatile uint32_t msgBitMask;
static volatile bool txInProgress;

// C-CAN callbacks
extern "C" {
    void C_CAN0_IRQHandler(void)
    {
        LPC_CAND_API->hwCAN_Isr(CanDriver::handle_);
    }

    void CAN_rx(uint8_t objNum) {
        // Blink LED from here, when RX operation is completed
        AdptLED::instance()->blinkRx();
        
        // Just set bitmask
        msgBitMask |= (1 << objNum);
    }

    void CAN_tx(uint8_t msgObjNum)
    {
        // Clear transmission in progress flag
        txInProgress = false;
        
        // Blink LED from here, when TX operation is completed
        AdptLED::instance()->blinkTx();
    }

    void CAN_error(uint32_t errorInfo)
    {
    }
}

/**
 * Make the particular message buffer part of the FIFO
 * @parameter msgobj C-CAN message object number
 */
static void SetFIFOItem(uint32_t msgobj) 
{   // p458,
    const uint32_t IFCREQ_BUSY = 0x8000;
    const uint32_t CMD_CTRL    = (1 << 4); // 1 is transfer the CTRL bit to the message object, 0 is not
    const uint32_t CMD_WR      = (1 << 7);
    const uint32_t DLC_LEN     = 8;
    const uint32_t RXIE        = (1 << 10);
    const uint32_t UMASK       = (1 << 12);

    // Write only control bits
    // Note: message number CANIF1_CMDREQ.MN starts with 1
    LPC_C_CAN0->CANIF1_CMDMSK_W = CMD_WR | CMD_CTRL;
    LPC_C_CAN0->CANIF1_MCTRL = DLC_LEN | RXIE | UMASK;
    LPC_C_CAN0->CANIF1_CMDREQ = msgobj + 1;    // Start message transfer
    while(LPC_C_CAN0->CANIF1_CMDREQ & IFCREQ_BUSY);
}

/**
 * Convert CAN_MSG_OBJ to CanMsgBuffer
 * @parameter   msg1   CAN_MSG_OBJ instance
 * @parameter   msg2   CanMsgBuffer instance 
 */
static void CanNative2Msg(const CAN_MSG_OBJ* msg1, CanMsgBuffer* msg2)
{
    msg2->id = msg1->mode_id & 0x1FFFFFFF;
    msg2->dlc = msg1->dlc;
    msg2->extended = (msg1->mode_id & CAN_MSGOBJ_EXT) ? true : false;
    memset(msg2->data, 0, 8);
    memcpy(msg2->data, msg1->data, msg1->dlc);
    msg2->msgnum = msg1->msgobj;
}

/**
 * Convert CanMsgBuffer to CAN_MSG_OBJ
 * @parameter   msg1   CanMsgBuffer instance
 * @parameter   msg2   CAN_MSG_OBJ instance
 */
static void CanMsg2Native(const CanMsgBuffer* msg1, CAN_MSG_OBJ* msg2, uint8_t msgobj)
{
    msg2->mode_id = msg1->id | (msg1->extended ? CAN_MSGOBJ_EXT : CAN_MSGOBJ_STD);
    msg2->mask = 0x0;
    msg2->dlc = msg1->dlc;
    memcpy(msg2->data, msg1->data, 8);
    msg2->msgobj = msgobj;
}

/**
 * Configuring CanDriver
 */
void CanDriver::configure()
{
    LPC_SYSCON->SYSAHBCLKCTRL1 |=  (1 << 7);
    LPC_SYSCON->PRESETCTRL1    |=  (1 << 7);
    LPC_SYSCON->PRESETCTRL1    &= ~(1 << 7);

    GPIOPinConfig(RxPort, RxPin, 0);
    GPIOPinConfig(TxPort, TxPin, 0);

    GPIOSetDir(RxPort, RxPin, GPIO_INPUT);
    GPIOSetDir(TxPort, TxPin, GPIO_OUTPUT);

    LPC_SWM->PINASSIGN6 &= 0xFF0000FF;
    LPC_SWM->PINASSIGN6 |= PinAssign;
}

/**
 * CanDriver singleton
 * @return The pointer to CandRiver instance
 */
CanDriver* CanDriver::instance()
{
    static CanDriver instance;
    return &instance;
}

/**
 * Intialize the CAN controller and interrupt handler
 */
CanDriver::CanDriver() : speed_(-1)
{
    // Enable the CAN Interrupt
    NVIC_EnableIRQ(C_CAN0_IRQn);
    msgBitMask = 0;
}

void CanDriver::setSpeed(int speed)
{
    const int MAX_CAN_PARAM_SIZE = 124;
    static uint32_t canApiMem[MAX_CAN_PARAM_SIZE];
    
    if(speed_ == speed) // Nothing to update
        return;

    // Initialize the CAN controller for ISO15765_CAN_500K/J1939_CAN_250K
    //
    //   ISO15765_CAN_500K: tq=125nS, T1=12, T2=3, SJW=3, 500 kBit/s
    //
    //   J1939_CAN_250K:    tq=250nS, T1=13, T2=2, SJW=1, 250 kBit/s
    //
    //-------------------------------------------------------------------
    const uint32_t btr = (speed == ISO15765_CAN_500K) ? 0x00002BC5 : 0x00001C4B;
    
    CAN_CFG canConfig = { 0, btr, 1 };
    CAN_CALLBACKS callbacks = { &CAN_rx, &CAN_tx, &CAN_error };
    
    CAN_API_INIT_PARAM_T apiInitCfg = {
        reinterpret_cast<uint32_t>(canApiMem),
        LPC_C_CAN0_BASE,
        &canConfig,
        &callbacks,
        nullptr,
        nullptr
    };

    if (LPC_CAND_API->hwCAN_Init(&handle_, &apiInitCfg) != 0) {
         while (1) {
            __WFI(); // Go to sleep
        }
    }
    speed_ = speed;
}

/**
 * Transmits a sequence of bytes to the ECU over CAN bus
 * @parameter   buff   CanMsgBuffer instance
 * @return the send operation completion status
 */
bool CanDriver::send(const CanMsgBuffer* buff)
{
    static CAN_MSG_OBJ msg;

    //Send CAN frame using msgobj=0
    CanMsg2Native(buff, &msg, 0);

    txInProgress = true;
    LPC_CAND_API->hwCAN_MsgTransmit(handle_, &msg);
    while (txInProgress)
        ;
    return true;
}

/**
 * Set the configuration for receiving messages
 *
 * @parameter   filter    CAN filter value
 * @parameter   mask      CAN mask value
 * @parameter   msgobj    C-CAN message object number
 * @parameter   extended  CAN extended message flag
 * @parameter   fifoLast  last FIFO message buffer flag
 */
void CanDriver::configRxMsgobj(uint32_t filter, uint32_t mask, uint8_t msgobj, bool extended, bool fifoLast)
{
    CAN_MSG_OBJ msg;

    msg.msgobj = msgobj;
    msg.mode_id = filter | (extended ? CAN_MSGOBJ_EXT : CAN_MSGOBJ_STD);
    msg.mask = mask;
    LPC_CAND_API->hwCAN_ConfigRxmsgobj(handle_, &msg);
    if (!fifoLast) {
        SetFIFOItem(msgobj);
    }
}

/**
 * Set the CAN filter/mask for FIFO buffer
 * @parameter   filter    CAN filter value
 * @parameter   mask      CAN mask value
 * @parameter   extended  CAN extended message flag
 * @return  the operation completion status
 */
bool CanDriver::setFilterAndMask(uint32_t filter, uint32_t mask, bool extended)
{
    // Set the FIFO buffer, starting with obj 1
    for (uint8_t msgobj = 1; msgobj <= FIFO_NUM; msgobj++) {
        bool fifoLast = (msgobj == FIFO_NUM);
        configRxMsgobj(filter, mask, msgobj, extended, fifoLast);
        if (!fifoLast) {
            SetFIFOItem(msgobj);
        }
    }
    return true;
}

/**
 * Set all FIFO filters to non-working combination to prevent receiving any messsages
 */
void CanDriver::clearFilters()
{
    setFilterAndMask(0x1FFFFFFF, 0x1FFFFFFF, true);
}

/**
 * Clear all the message buffers except 0, used for transmit
 */
void CanDriver::clearData()
{
    CAN_MSG_OBJ msg;
    for (int i = 1; i < 32; i++) {
        msg.dlc = msg.mode_id = 0;
        msg.msgobj = i;
        LPC_CAND_API->hwCAN_MsgReceive(CanDriver::handle_, &msg);
    }
    msgBitMask = 0;
}

/**
 * Set a single CAN filter/mask
 * @parameter   filter    CAN filter value
 * @parameter   mask      CAN mask value
 * @parameter   extended  CAN extended message flag
 * @parameter   msgobj    CAN message object number
 * @return  the operation completion status
 */
bool CanDriver::setFilterAndMask(uint32_t filter, uint32_t mask, bool extended, int msgobj)
{
    if (msgobj > 0 && msgobj < 6) {
        configRxMsgobj(filter, mask, msgobj, extended, true);
        return true;
    }
    return false;
}

/**
 * Read the CAN frame from FIFO buffer
 * @return  true if read the frame / false if no frame
 */
bool CanDriver::read(CanMsgBuffer* buff)
{
    CAN_MSG_OBJ msg;
    msg.mode_id = 0xFFFFFFFF;
    msg.dlc = 0;
    uint32_t mask = msgBitMask;
    for (int i = 1; i < 32; i++) {
        uint32_t val = 1 << i;
        if (val & mask) {
            msgBitMask &= ~val;
            msg.msgobj = i;
            LPC_CAND_API->hwCAN_MsgReceive(CanDriver::handle_, &msg);
            CanNative2Msg(&msg, buff);
            return (msg.mode_id != 0xFFFFFFFF);
        }
    }
    return false;
}

/**
 * Read CAN frame received status
 * @return  true/false
 */
bool CanDriver::isReady() const
{
    return (msgBitMask != 0L);
}

/**
 * Wakes up the CAN peripheral from sleep mode
 * @return  true/false
 */
bool CanDriver::wakeUp()
{
    return true;
}

/**
 * Enters the sleep (low power) mode
 * @return  true/false
 */
bool CanDriver::sleep()
{
    return false;
}

/**
 * Switch on/off CAN and let the CAN pins controlled directly (testing mode)
 * @parameter  val  CAN testing mode flag 
 */
void CanDriver::setBitBang(bool val)
{
    if (!val) {
        LPC_SWM->PINASSIGN6 &= 0xFF0000FF;
        LPC_SWM->PINASSIGN6 |= PinAssign;
    }
    else {
        LPC_SWM->PINASSIGN6 |= 0x00FFFF00;
    }
}

/**
 * Set the CAN transmitter pin status
 * @parameter  bit  CAN TX pin value
 */
void CanDriver::setBit(uint32_t bit)
{
    GPIOPinWrite(TxPort, TxPin, bit);
}

/**
 * Read CAN RX pin status
 * @return pin status, 1 if set, 0 otherwise
 */
uint32_t CanDriver::getBit()
{
    return GPIOPinRead(RxPort, RxPin);
}

/**
 * Switch on/off CAN and let the CAN pins controlled directly (testing mode)
 * @parameter  val  CAN silent mode flag 
 */
void CanDriver::setSilent(bool val)
{
    const uint32_t CANCTRL_TEST   = (1 << 7); // CAN CTRL register
    const uint32_t CANTEST_SILENT = (1 << 3); // CAN TEST register

    if (val) {
        LPC_C_CAN0->CANCNTL |= CANCTRL_TEST;   // Enable test mode
        LPC_C_CAN0->CANTEST |= CANTEST_SILENT; // Enable silent
    }
    else {
        LPC_C_CAN0->CANCNTL &= ~CANCTRL_TEST;  // Disable test mode
    }
}
