#ifndef HNODE_SWITCH_INTERFACE_H_
#define HNODE_SWITCH_INTERFACE_H_

enum SwitchInterfacePacketTypes
{
    SWPKT_SWITCH_LIST_REQUEST               = 1,
    SWPKT_SWITCH_LIST_REPLY                 = 2,
    SWPKT_SWITCH_STATE_REQUEST              = 3,
    SWPKT_SWITCH_STATE_REPLY                = 4,
    SWPKT_SWITCH_CMD_REQUEST                = 5,
    SWPKT_SWITCH_CMD_REPLY                  = 6,
    SWPKT_SWITCH_REPORT_SETTING_REQUEST     = 7,
    SWPKT_SWITCH_REPORT_SETTING_REPLY       = 8,
    SWPKT_SWITCH_SET_SETTING_REQUEST        = 9,
    SWPKT_SWITCH_SET_SETTING_REPLY          = 10,
    SWPKT_SWITCH_REPORT_SCHEDULE_REQUEST    = 11,
    SWPKT_SWITCH_REPORT_SCHEDULE_REPLY      = 12,
    SWPKT_SWITCH_SET_SCHEDULE_REQUEST       = 13,
    SWPKT_SWITCH_SET_SCHEDULE_REPLY         = 14,
};

enum SwitchInterfaceSwitchCapabilityFlags
{
    SWINF_CAPFLAGS_ONOFF        = 0x00000001,  // Switch can be turned on and off
    SWINF_CAPFLAGS_STEP_DIM     = 0x00000002,  // Switch is dimmable in steps (ex. X10 model)
    SWINF_CAPFLAGS_SET_DIM      = 0x00000004,  // Switch is dimmable by setting a dim value
    SWINF_CAPFLAGS_REPORT_STATE = 0x00000008,  // Switch is capable of reporting it's current state.
    SWINF_CAPFLAGS_EVENT        = 0x00000010,  // Interface can generate an HNode Event when switch state changes.
};

enum SwitchInterfaceSwitchCommandCodes
{
    SWINF_CMD_TURN_ON    = 1,
    SWINF_CMD_TURN_OFF   = 2,
};

#endif // HNODE_SWITCH_INTERFACE
