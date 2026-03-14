// Copyright (c) 2023-2026 Dan O’Malley
// This file is licensed under the MIT License. See LICENSE for details.
// Text descriptions generated with Grok by xAI.
// 12/2025 with Grok v4.


#include "constants.h"

/**
 * Structure representing parameters for network operations, particularly for UDP sending.
 * This struct encapsulates all necessary information for constructing and sending a network packet.
 */
struct networkParameter
{
    /** The source IP address in network byte order. */
    uint32_t sourceIPAddress;
    /** The destination IP address in network byte order. */
    uint32_t destinationIPAddress; 
    /** The source port number in host byte order. */
    uint16_t sourcePort;
    /** The destination port number in host byte order. */
    uint16_t destinationPort;
    /** Pointer to the socket name or identifier as a null-terminated string. */
    uint8_t *socketName;
    /** Pointer to the packet payload data, which is the message to be sent. */
    uint8_t *packetPayload;
};

/**
 * Computes the checksum for a given data buffer, which is used for IP and UDP headers.
 * This function accumulates the sum of 16-bit words from the data, handles any odd byte,
 * folds the carry bits, and returns the one's complement of the result.
 *
 * @param data Pointer to the data buffer to checksum.
 * @param len Length of the data buffer in bytes.
 * @return The computed checksum (one's complement of the folded sum).
 */
uint16_t checkSum(uint8_t *data, uint32_t len);

/**
 * Initializes the NE2000 network interface card (NIC) by resetting it,
 * configuring registers for 16-bit mode, setting up the receive ring buffer,
 * programming the MAC address, and starting the NIC in a ready state for
 * receiving broadcast packets without interrupts (polling mode).
 * This function ensures the NIC is properly set up for basic network operations.
 */
void netInit();

/**
 * Receives UDP packets from the NE2000 NIC by polling the receive ring buffer.
 * Processes all pending packets: handles ARP requests by sending replies,
 * queues UDP payloads from IP packets, and updates network statistics.
 * After processing, combines queued UDP payloads into a single buffer separated by newlines.
 * Clears incoming buffers before processing and manages a queue to handle multiple packets.
 */
void netUDPRcv();

/**
 * Sends a UDP packet over the NE2000 NIC. Constructs an Ethernet frame with IP and UDP headers,
 * computes checksums for IP and UDP, pads the frame to minimum length if necessary,
 * writes the packet to the NIC's transmit buffer, initiates transmission,
 * and updates the transmitted packet count.
 * Returns early if the payload would exceed maximum frame length.
 *
 * @param NetworkParameter Pointer to a structure containing source/destination IP, ports, and payload.
 */
void netUDPSend(struct networkParameter *NetworkParameter);