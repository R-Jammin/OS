// Copyright (c) 2025-2026 Dan O’Malley
// This file is licensed under the MIT License. See LICENSE for details.
// Generated with Grok by xAI.
// 12/2025 with Grok v4.


#include "constants.h"
#include "x86.h"
#include "exceptions.h"
#include "libc-main.h"
#include "vm.h"
#include "net.h"

// https://wiki.osdev.org/Ne2000

// Use netcat to listen on port 1234 "nc -u -l -p 1234"
// You can use tcpdump to see the data "sudo tcpdump -i any udp port 1234 -vv -X"
// I made QEMU create a dump.pcap file to capture as well "tcpdump -r dump.pcap"
// Can automatically support data payload/message of around 1400 bytes.
// nc -u 127.0.0.1 12345 

// Byte swap (comes from htons and htonl in standard libc)
#define hostToNetworkShort(x) (((x & 0xFF) << 8) | ((x >> 8) & 0xFF))
#define hostToNetworkLong(x) ((hostToNetworkShort(x & 0xFFFF) << 16) | hostToNetworkShort(x >> 16))

#define MIN_PACKET_LEN 60 
#define MAX_PACKET_LEN 1514

/**
 * Computes the checksum for a given data buffer, which is used for IP and UDP headers.
 * This function accumulates the sum of 16-bit words from the data, handles any odd byte,
 * folds the carry bits, and returns the one's complement of the result.
 *
 * @param data Pointer to the data buffer to checksum.
 * @param len Length of the data buffer in bytes.
 * @return The computed checksum (one's complement of the folded sum).
 */
uint16_t checkSum(uint8_t *data, uint32_t len)
{
    uint32_t sum = 0;
    for (uint32_t i = 0; i < len; i += 2)
    {
        sum += (data[i] << 8) | (i + 1 < len ? data[i+1] : 0);
    }
    while (sum >> 16)
    {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    return ~sum;
}

/**
 * Initializes the NE2000 network interface card (NIC) by resetting it,
 * configuring registers for 16-bit mode, setting up the receive ring buffer,
 * programming the MAC address, and starting the NIC in a ready state for
 * receiving broadcast packets without interrupts (polling mode).
 * This function ensures the NIC is properly set up for basic network operations.
 */
void netInit()
{
    uint16_t ioBasePort = NE2000_BASE_PORT;
    
    // Reset NIC
    uint8_t resetValue = inputIOPort(ioBasePort + 0x1F);
    outputIOPort(ioBasePort + 0x1F, resetValue);
    
    uint32_t resetTimeout = 1000000;  // Adjust based on your system; prevents infinite loop
    while ((inputIOPort(ioBasePort + 0x07) & 0x80) == 0 && resetTimeout > 0)
    {
        resetTimeout--;
    }
    if (resetTimeout == 0)
    {
        // Optional: Log "NIC reset timed out" or handle error; proceeding for now
    }
    
    outputIOPort(ioBasePort + 0x07, 0xFF);

    // Stop, data config (16-bit mode)
    outputIOPort(ioBasePort + NE_CR, 0x01);  // Page 0, stop
    outputIOPort(ioBasePort + NE_RBCR0, 0x00);
    outputIOPort(ioBasePort + NE_RBCR1, 0x00);
    outputIOPort(ioBasePort + NE_DCR, 0x49);  // Word mode, little endian, normal operation (no loopback)

    // Set page start/stop/bound (basic ring)
    outputIOPort(ioBasePort + NE_PSTART, 0x46);
    outputIOPort(ioBasePort + NE_PSTOP, 0x80);
    outputIOPort(ioBasePort + NE_BOUND, 0x45);

    // Set MAC address and CURR on page 1
    uint8_t guestMacAddress[6] = {0x52, 0x54, 0x00, 0x12, 0x34, 0x56};
    outputIOPort(ioBasePort + NE_CR, 0x41);  // Switch to page 1, stop
    for (int i = 0; i < 6; i++)
    {
        outputIOPort(ioBasePort + 0x01 + i, guestMacAddress[i]);  // PAR0 to PAR5
    }
    outputIOPort(ioBasePort + 0x07, 0x46);   // Set CURR to PSTART
    outputIOPort(ioBasePort + NE_CR, 0x01);  // Switch back to page 0, stop

    // RX: Broadcast
    outputIOPort(ioBasePort + NE_RCR, 0x04);

    // TX: Normal
    outputIOPort(ioBasePort + NE_TCR, 0x00);

    // Disable interrupts (poll-only)
    outputIOPort(ioBasePort + NE_IMR, 0x00);

    // Start
    outputIOPort(ioBasePort + NE_CR, 0x02);
}

/**
 * Receives UDP packets from the NE2000 NIC by polling the receive ring buffer.
 * Processes all pending packets: handles ARP requests by sending replies,
 * queues UDP payloads from IP packets, and updates network statistics.
 * After processing, combines queued UDP payloads into a single buffer separated by newlines.
 * Clears incoming buffers before processing and manages a queue to handle multiple packets.
 */
void netUDPRcv()
{
    static uint8_t guestMacAddress[6] = {0x52, 0x54, 0x00, 0x12, 0x34, 0x56};
    #define GUEST_IP 0x0A00020F  // 10.0.2.15 in host byte order

    #define MAX_QUEUED_PACKETS 10

    static uint8_t payloadQueue[MAX_QUEUED_PACKETS][MAX_PACKET_LEN];
    static uint16_t payloadLengthQueue[MAX_QUEUED_PACKETS];
    static uint32_t queueSize = 0;
    static uint32_t queueFront = 0;
    static uint32_t queueRear = 0;


    fillMemory((uint8_t*)NETWORK_INCOMING_RCV_BUFFER, 0x0, NETWORK_INCOMING_RCV_BUFFER_SIZE);
    fillMemory((uint8_t*)NETWORK_INCOMING_PAYLOAD_BUFFER, 0x0, NETWORK_INCOMING_PAYLOAD_BUFFER_SIZE);

    uint8_t boundaryPage = inputIOPort(NE2000_BASE_PORT + 0x03);
    outputIOPort(NE2000_BASE_PORT + 0x00, 0x42);  // Page 1, start (to read CURR)
    uint8_t currentPage = inputIOPort(NE2000_BASE_PORT + 0x07);
    outputIOPort(NE2000_BASE_PORT + 0x00, 0x02);  // Page 0, start

    uint8_t receivePage = boundaryPage + 1;
    if (receivePage == 0x80)
    {
        receivePage = 0x46;
    }

    bool processedUdp = false;

    // Process all pending packets in the ring
    while (receivePage != currentPage)
    {
        outputIOPort(NE2000_BASE_PORT + 0x0A, 4);
        outputIOPort(NE2000_BASE_PORT + 0x0B, 0);
        outputIOPort(NE2000_BASE_PORT + 0x08, 0);
        outputIOPort(NE2000_BASE_PORT + 0x09, receivePage);
        outputIOPort(NE2000_BASE_PORT + 0x00, 0x0A);

        uint8_t receiveHeader[4];
        ioPortWordToMem(NE2000_BASE_PORT + 0x10, receiveHeader, 2);

        // Wait for RDC and ack
        for (uint32_t i = 0; i < 100000; i++)
        {
            if (inputIOPort(NE2000_BASE_PORT + NE_ISR) & 0x40)
            {
                break;
            }
        }
        outputIOPort(NE2000_BASE_PORT + NE_ISR, 0x40);

        uint8_t nextPage = receiveHeader[1];
        uint16_t packetLength = (receiveHeader[3] << 8) | receiveHeader[2];

        // Validate receive status (receiveHeader[0])
        if ((receiveHeader[0] & 0x01) != 0x01 || (receiveHeader[0] & 0x0E) != 0x00 || packetLength < 64 || packetLength > MAX_PACKET_LEN + 4)
        {
            // Invalid packet: skip it
            uint8_t newBoundary = nextPage - 1;
            if (newBoundary < 0x46)
            {
                newBoundary = 0x7F;
            }
            outputIOPort(NE2000_BASE_PORT + 0x03, newBoundary);
            outputIOPort(NE2000_BASE_PORT + 0x07, 0x03);  // Ack PRX and RXE if error

            // Update for next
            boundaryPage = newBoundary;
            receivePage = boundaryPage + 1;
            if (receivePage == 0x80)
            {
                receivePage = 0x46;
            }
            continue;
        }

        // Read the packet data, skipping the 4-byte header
        uint16_t dataLength = packetLength - 4;
        outputIOPort(NE2000_BASE_PORT + 0x0A, dataLength & 0xFF);
        outputIOPort(NE2000_BASE_PORT + 0x0B, dataLength >> 8);
        outputIOPort(NE2000_BASE_PORT + 0x08, 4);  // Start after header
        outputIOPort(NE2000_BASE_PORT + 0x09, receivePage);
        outputIOPort(NE2000_BASE_PORT + 0x00, 0x0A);

        uint8_t ethernetPacket[MAX_PACKET_LEN];
        ioPortWordToMem(NE2000_BASE_PORT + 0x10, ethernetPacket, (dataLength + 1) / 2);

        // Wait for RDC and ack
        for (uint32_t i = 0; i < 100000; i++)
        {
            if (inputIOPort(NE2000_BASE_PORT + NE_ISR) & 0x40)
            {
                break;
            }
        }
        outputIOPort(NE2000_BASE_PORT + NE_ISR, 0x40);

        uint8_t newBoundary = nextPage - 1;
        if (newBoundary < 0x46)
        {
            newBoundary = 0x7F;
        }
        outputIOPort(NE2000_BASE_PORT + 0x03, newBoundary);

        outputIOPort(NE2000_BASE_PORT + 0x07, 0x01);

        // Process the packet
        uint16_t etherType = (ethernetPacket[12] << 8) | ethernetPacket[13];
        if (etherType == 0x0806)
        {
            // Handle ARP
            uint16_t arpOpcode = (ethernetPacket[20] << 8) | ethernetPacket[21];
            if (arpOpcode == 1)
            {
                uint32_t targetIp = (ethernetPacket[38] << 24) | (ethernetPacket[39] << 16) | (ethernetPacket[40] << 8) | ethernetPacket[41];
                if (targetIp == GUEST_IP)
                {
                    // Build ARP reply
                    uint8_t arpReply[60];
                    fillMemory(arpReply, 0, 60);

                    // Ethernet header
                    bytecpy(arpReply, ethernetPacket + 6, 6);  // dst = orig src MAC
                    bytecpy(arpReply + 6, guestMacAddress, 6);  // src = guest MAC
                    arpReply[12] = 0x08;
                    arpReply[13] = 0x06;

                    // ARP header
                    arpReply[14] = 0x00;
                    arpReply[15] = 0x01;
                    arpReply[16] = 0x08;
                    arpReply[17] = 0x00;
                    arpReply[18] = 0x06;
                    arpReply[19] = 0x04;
                    arpReply[20] = 0x00;
                    arpReply[21] = 0x02;
                    bytecpy(arpReply + 22, guestMacAddress, 6);  // sender MAC = guest
                    arpReply[28] = (GUEST_IP >> 24) & 0xFF;
                    arpReply[29] = (GUEST_IP >> 16) & 0xFF;
                    arpReply[30] = (GUEST_IP >> 8) & 0xFF;
                    arpReply[31] = GUEST_IP & 0xFF;  // sender IP = guest
                    bytecpy(arpReply + 32, ethernetPacket + 22, 6);  // target MAC = orig sender MAC
                    bytecpy(arpReply + 38, ethernetPacket + 28, 4);  // target IP = orig sender IP

                    // Send the reply
                    uint16_t sendLength = 60;
                    outputIOPort(NE2000_BASE_PORT + NE_TPSR, 0x40);
                    outputIOPort(NE2000_BASE_PORT + NE_TBCR0, sendLength & 0xFF);
                    outputIOPort(NE2000_BASE_PORT + NE_TBCR1, sendLength >> 8);

                    outputIOPort(NE2000_BASE_PORT + NE_CR, 0x12);
                    outputIOPort(NE2000_BASE_PORT + NE_RSAR0, 0x00);
                    outputIOPort(NE2000_BASE_PORT + NE_RSAR1, 0x40);
                    outputIOPort(NE2000_BASE_PORT + NE_RBCR0, sendLength & 0xFF);
                    outputIOPort(NE2000_BASE_PORT + NE_RBCR1, sendLength >> 8);
                    memToIoPortWord(NE2000_BASE_PORT + 0x10, arpReply, (sendLength + 1) / 2);

                    // Wait for RDC and ack
                    for (uint32_t i = 0; i < 100000; i++)
                    {
                        if (inputIOPort(NE2000_BASE_PORT + NE_ISR) & 0x40)
                        {
                            break;
                        }
                    }
                    outputIOPort(NE2000_BASE_PORT + NE_ISR, 0x40);

                    // Start transmit
                    outputIOPort(NE2000_BASE_PORT + NE_CR, 0x26);

                    // Wait for PTX and ack
                    for (uint32_t i = 0; i < 100000; i++)
                    {
                        if (inputIOPort(NE2000_BASE_PORT + NE_ISR) & 0x02)
                        {
                            break;
                        }
                    }

                    outputIOPort(NE2000_BASE_PORT + NE_ISR, 0x02);

                    // Increment transmitted count
                    uint32_t networkPacketsTransmittedCount = *(uint32_t*)NETWORK_PACKETS_TRANSMITTED;
                    networkPacketsTransmittedCount++;
                    *(uint32_t*)(NETWORK_PACKETS_TRANSMITTED) = networkPacketsTransmittedCount;
                }
            }
            // Continue to next packet without returning
        }
        else if (etherType == 0x0800)
        {
            // Assume IP/UDP packet; copy to buffer
            fillMemory((uint8_t*)NETWORK_INCOMING_RCV_BUFFER, 0, MAX_PACKET_LEN);
            bytecpy((uint8_t*)NETWORK_INCOMING_RCV_BUFFER, ethernetPacket, dataLength);

            // Extract payload
            uint8_t *ipHeader = ethernetPacket + 14;
            uint8_t ipHeaderLength = (ipHeader[0] & 0x0F) * 4;
            if (ipHeaderLength < 20)
            {
                // Invalid IP header, skip processing UDP
                boundaryPage = newBoundary;
                receivePage = boundaryPage + 1;
                if (receivePage == 0x80)
                {
                    receivePage = 0x46;
                }
                continue;
            }
            uint8_t ipProtocol = ipHeader[9];
            if (ipProtocol != 0x11)
            {
                // Not UDP, skip
                boundaryPage = newBoundary;
                receivePage = boundaryPage + 1;
                if (receivePage == 0x80)
                {
                    receivePage = 0x46;
                }
                continue;
            }
            uint8_t *udpHeader = ipHeader + ipHeaderLength;
            uint16_t udpLength = hostToNetworkShort(*((uint16_t *)(udpHeader + 4)));
            if (udpLength < 8)
            {
                // Invalid UDP length, skip
                boundaryPage = newBoundary;
                receivePage = boundaryPage + 1;
                if (receivePage == 0x80)
                {
                    receivePage = 0x46;
                }
                continue;
            }
            uint8_t *udpPayload = udpHeader + 8;
            uint16_t udpPayloadLength = udpLength - 8;

            if (queueSize < MAX_QUEUED_PACKETS)
            {
                bytecpy(payloadQueue[queueRear], udpPayload, udpPayloadLength);
                payloadLengthQueue[queueRear] = udpPayloadLength;
                queueRear = (queueRear + 1) % MAX_QUEUED_PACKETS;
                queueSize++;
                processedUdp = true;
            }
            // else drop the packet

            // Increment received count
            uint32_t networkPacketsReceivedCount = *(uint32_t*)NETWORK_PACKETS_RECEIVED;
            networkPacketsReceivedCount++;
            *(uint32_t*)NETWORK_PACKETS_RECEIVED = networkPacketsReceivedCount;

        }  // Else, ignore other types and continue

        // Update for next
        boundaryPage = newBoundary;
        receivePage = boundaryPage + 1;
        if (receivePage == 0x80)
        {
            receivePage = 0x46;
        }
    }

    if (processedUdp)
    {
        uint8_t *ptr = (uint8_t*)NETWORK_INCOMING_PAYLOAD_BUFFER;
        uint32_t idx = queueFront;
        for (uint32_t i = 0; i < queueSize; i++)
        {
            bytecpy(ptr, payloadQueue[idx], payloadLengthQueue[idx]);
            ptr += payloadLengthQueue[idx];
            if (i < queueSize - 1)
            {
                *ptr++ = '\n';
            }
            idx = (idx + 1) % MAX_QUEUED_PACKETS;
        }

        queueSize = 0;
        queueFront = 0;
        queueRear = 0;
        fillMemory((uint8_t*)payloadQueue, 0, sizeof(payloadQueue));
        fillMemory((uint8_t*)payloadLengthQueue, 0, sizeof(payloadLengthQueue));

        return;
    }


}

/**
 * Sends a UDP packet over the NE2000 NIC. Constructs an Ethernet frame with IP and UDP headers,
 * computes checksums for IP and UDP, pads the frame to minimum length if necessary,
 * writes the packet to the NIC's transmit buffer, initiates transmission,
 * and updates the transmitted packet count.
 * Returns early if the payload would exceed maximum frame length.
 *
 * @param NetworkParameter Pointer to a structure containing source/destination IP, ports, and payload.
 */
void netUDPSend(struct networkParameter *NetworkParameter) 
{
    uint32_t payloadLength = strlen(NetworkParameter->packetPayload) + 1;  // Include null terminator
    uint16_t udpLength = 8 + payloadLength;
    uint16_t ipLength = 20 + udpLength;
    uint16_t frameLength = 14 + ipLength;
    uint32_t networkPacketsTransmittedCount = *(uint32_t*)NETWORK_PACKETS_TRANSMITTED;

    if (frameLength > MAX_PACKET_LEN)
    {
        return;
    }

    uint32_t sendLength = frameLength < MIN_PACKET_LEN ? MIN_PACKET_LEN : frameLength;

    uint8_t packet[MAX_PACKET_LEN];
    fillMemory(packet, 0, sendLength);

    // Layer 2
    // Ethernet header
    uint8_t destinationMacAddress[6] = {0x02, 0x00, 0xA0, 0x04, 0x00, 0x00};
    bytecpy(packet, destinationMacAddress, 6);
    uint8_t sourceMacAddress[6] = {0x52, 0x54, 0x00, 0x12, 0x34, 0x56};
    bytecpy(packet + 6, sourceMacAddress, 6);
    *((uint16_t *)(packet + 12)) = hostToNetworkShort(0x0800); // IPv4

    // Layer 3
    // IP header
    uint8_t *ipHeader = packet + 14;
    ipHeader[0] = 0x45; // Version
    ipHeader[1] = 0x00;
    *((uint16_t *)(ipHeader + 2)) = hostToNetworkShort(ipLength);
    *((uint16_t *)(ipHeader + 4)) = hostToNetworkShort(0x0000); // ID
    *((uint16_t *)(ipHeader + 6)) = hostToNetworkShort(0x4000); // Don't fragment
    ipHeader[8] = 0x40; // TTL
    ipHeader[9] = 0x11; // UDP -> 0x11 = 17
    *((uint32_t *)(ipHeader + 12)) = hostToNetworkLong(NetworkParameter->sourceIPAddress);
    *((uint32_t *)(ipHeader + 16)) = hostToNetworkLong(NetworkParameter->destinationIPAddress);
    // Checksum placeholder
    *((uint16_t *)(ipHeader + 10)) = 0;

    // Layer 4
    // UDP header
    uint8_t *udpHeader = ipHeader + 20;
    *((uint16_t *)(udpHeader + 0)) = hostToNetworkShort(NetworkParameter->sourcePort);
    *((uint16_t *)(udpHeader + 2)) = hostToNetworkShort(NetworkParameter->destinationPort);
    *((uint16_t *)(udpHeader + 4)) = hostToNetworkShort(udpLength);
    *((uint16_t *)(udpHeader + 6)) = 0;
    bytecpy(udpHeader + 8, NetworkParameter->packetPayload, payloadLength);

    // Compute IP checksum
    uint16_t ipChecksum = checkSum(ipHeader, 20);
    *((uint16_t *)(ipHeader + 10)) = hostToNetworkShort(ipChecksum);

    // Compute UDP checksum
    uint8_t pseudoHeader[12 + udpLength];
    fillMemory(pseudoHeader, 0, 12 + udpLength);
    *((uint32_t *)(pseudoHeader + 0)) = hostToNetworkLong(NetworkParameter->sourceIPAddress);
    *((uint32_t *)(pseudoHeader + 4)) = hostToNetworkLong(NetworkParameter->destinationIPAddress);
    pseudoHeader[9] = 0x11;
    *((uint16_t *)(pseudoHeader + 10)) = hostToNetworkShort(udpLength);
    bytecpy(pseudoHeader + 12, udpHeader, udpLength);
    uint16_t udpChecksum = checkSum(pseudoHeader, 12 + udpLength);
    if (udpChecksum == 0)
    {
        udpChecksum = 0xFFFF;
    }
    *((uint16_t *)(udpHeader + 6)) = hostToNetworkShort(udpChecksum);

    // TX
    outputIOPort(NE2000_BASE_PORT + NE_TPSR, 0x40);
    outputIOPort(NE2000_BASE_PORT + NE_TBCR0, sendLength & 0xFF);
    outputIOPort(NE2000_BASE_PORT + NE_TBCR1, sendLength >> 8);

    outputIOPort(NE2000_BASE_PORT + NE_CR, 0x12);
    outputIOPort(NE2000_BASE_PORT + NE_RSAR0, 0x00);
    outputIOPort(NE2000_BASE_PORT + NE_RSAR1, 0x40);
    outputIOPort(NE2000_BASE_PORT + NE_RBCR0, sendLength & 0xFF);
    outputIOPort(NE2000_BASE_PORT + NE_RBCR1, sendLength >> 8);
    memToIoPortWord(NE2000_BASE_PORT + 0x10, packet, (sendLength + 1) / 2);

    // Wait for RDC and ack
    for (uint32_t i = 0; i < 100000; i++)
    {
        if (inputIOPort(NE2000_BASE_PORT + NE_ISR) & 0x40)
        {
            break;
        }
    }
    outputIOPort(NE2000_BASE_PORT + NE_ISR, 0x40);

    // Start transmit
    outputIOPort(NE2000_BASE_PORT + NE_CR, 0x26);

    // Wait for PTX and ack
    for (uint32_t i = 0; i < 100000; i++)
    {
        if (inputIOPort(NE2000_BASE_PORT + NE_ISR) & 0x02)
        {
            break;
        }
    }

    outputIOPort(NE2000_BASE_PORT + NE_ISR, 0x02);

    networkPacketsTransmittedCount++;
    *(uint32_t*)(NETWORK_PACKETS_TRANSMITTED) = networkPacketsTransmittedCount;
}