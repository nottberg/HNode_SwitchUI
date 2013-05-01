#ifndef __HNODE_DEBUG_H__
#define __HNODE_DEBUG_H__

typedef unsigned char   U8_T;
typedef unsigned short  U16_T;
typedef unsigned long   U32_T;

typedef struct HNodeDebugPacket
{
    U16_T   PktType;
    U16_T   PktSeq;
    U16_T   DataLength;
    U8_T    Data[1];
}HNODE_DEBUG_PKT;

typedef struct HNodeDebugDeadEntryElement
{
    U8_T    Id;
    U8_T    Param1;
    U16_T   Param2;
}HNODE_DEAD_ENTRY;

typedef struct ConfigurationDataBaseHeader
{
    U8_T   Length[2];       // The length of the config data structure.  Includes both this field and the CRC field at the end.
    U8_T   BaseLength[2];   // The length of the base code configuration data.
    U8_T   MajorVersion;    // The major version of the base config data format. Bumped for incompatible changes. 
    U8_T   MinorVersion;    // The minor version of the base config data format. Bumped for upwardly compatible changes.
    U8_T   NodeUID[16];     // Node identifier.  First 6-bytes also represent the ethernet address of the device.
    U8_T   DNSSDName[45];   // The prefix name for use by the DNS-SD protocol.
}CFG_DATA_BASE_HDR;

#endif // __HNODE_DEBUG_H__
