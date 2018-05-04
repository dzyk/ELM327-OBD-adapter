/**
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009-2018 ObdDiag.Net. All rights reserved.
 *
 */

#ifndef __OBD_PROFILE_H__ 
#define __OBD_PROFILE_H__

#include "padapter.h"

class DataCollector;

class OBDProfile {
private:
    OBDProfile();
public:
    static OBDProfile* instance();
    void getProfileDescription() const;
    void getProtocolDescription() const;
    void getProtocolDescriptionNum() const;
    int setProtocol(int protocol, bool refreshConnection);
    void sendHeartBeat();
    void dumpBuffer();
    void closeProtocol();
    void onRequest(const DataCollector* collector);
    int getProtocol() const;
    void wiringCheck();
    int kwDisplay();
    void setFilterAndMask();
    void monitor();
    void monitor(const util::string& cmdString);
private:
    bool sendLengthCheck(int len);
    int onRequestImpl(const DataCollector* collector);
    ProtocolAdapter* adapter_;
};

#endif //__OBD_PROFILE_H__
