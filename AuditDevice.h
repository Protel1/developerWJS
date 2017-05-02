/**********************************************************************************************************************
 *                                      This file contains the AuditDevice class.                                     *
 *                                                                                                                    *
 * This class is, in essence, a struct used by ProtelDevice and ProtelHost to hold data returned by a master auditor  *
 * in its N command response. This response holds data for the master auditor itself and for slave devices connected  *
 * to it: data for each device is placed in a separate AuditDevice instance.                                          *
 *                                                                                                                    *
 * The struct is set up so that the device data is automatically parsed when ProtelHost uses                          *
 * ProtelDevice::SetAuditDevice to copy the relevant part of the N response over the struct. Required parts of the    *
 * data (e.g. StatusByte) can then be retrieved directly.                                                             *
 *                                                                                                                    *
 *                                         Copyright (c) Protel Inc. 2009-2010                                        *
 **********************************************************************************************************************/
#pragma once

/*
 * Details of N command - – (New Audit Device Numbers needing configuration- 4E hex)
 *
 * The ‘N’ command will be used to request the serial numbers of all devices assigned to the master. The Master will
 * build a data table based on the following format using the serial numbers of the monitors, their assigned address,
 * the firmware version it is running, Connection type, and a status byte. No delimiters will be used.
 *
 * Two additional bytes, Y1 and Y2 are added at the beginning of each packet of data. This addition, Y1/Y2, is a
 * 2-byte packet count starting with 0001h, Y1 being the most significant byte. The Host will be driving the request
 * by asking for each packet, then incrementing its packet number and requesting the next packet, until it receives a
 * Y1\Y2 equal to FFFFh. The Host can request a re-send of a packet if an error occurs by reissuing the request for
 * the same packet number.
 *
 *
 * Host Sends-
 *  ‘T’  |  3h  |  ‘N’ | Y1h  |  Y2h  |  Checksum (h)
 *
 * Response from Master Auditor- The Master Auditor sends an error response or begins a ‘N’ response as follows:
 *  ‘T’  |  Length (h)  |  ‘N’ |  Z1h  |  Z2h  |  Data  |  Checksum (h)
 *
 * Z1/Z2 is a 2-byte packet counter that starts with 0001h, Z1 being the most significant byte. The packet count is
 * set to FFFFh to indicate the last packet.
 *
 * Data Table:
 *  Security number     8 bytes alphanumeric data in ASCII
 *  Address         1 byte in hex
 *  Firmware Version Level  3 bytes ASCII
 *  (First byte indicates Audit (‘A’), Beta (‘B’), or Production (‘0’)
 *  Firmware Monitor Type   1 byte ASCII
 *  (# = AVW, . = Food  & Beverage,$ = GSM Food & Beverage)
 *  Firmware Version Rev.   2 bytes ASCII
 *  Config File Version 1 byte ASCII
 *  Connection type     1 byte 1=Hard wire, 2=Radio
 *  Status byte     Bit 0 = 1 =Remote / 0 = Central
 *   Bit 1 =1 = Display unit
 *   Bit 2 – 6 – Not used at this time
 *   Bit 7 =1 = Auditor Needs Configuration file
 */

#pragma pack(1)
struct AuditDevice
 {
    char szSerialNumber [ 8 ];
    BYTE Address;
    char FirmwareVersionLevel [ 3 ];
    BYTE FirmwareMonitorType;
    BYTE FirmwareVersionRev [ 2 ];
    BYTE ConfigFileVersion;
    BYTE ConnectionType;
    BYTE StatusByte;
 };
#pragma pack()
