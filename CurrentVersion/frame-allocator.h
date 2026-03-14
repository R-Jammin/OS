// Copyright (c) 2023-2026 Dan O’Malley
// This file is licensed under the MIT License. See LICENSE for details.
// Text descriptions generated with Grok by xAI.
// 12/2025 with Grok v4.


/** Creates the initial structure of the page frame map.
 * \param pageFrameMap The pointer to the beginning of the map.
 * \param numberOfFrame How many frames to create. 
 */
void createPageFrameMap(uint8_t *pageFrameMap, uint32_t numberOfFrames);

/** Allocates a frame in the page frame map and returns the frame number.
 * \param pid The pid who is requesting the frame.
 * \param pageFrameMap The pointer to the beginning of the map.
 */
uint32_t allocateFrame(uint32_t pid, uint8_t *pageFrameMap);

/** Frees a frame in the page frame map.
 * \param frameNumber The frame to free.
 */
void freeFrame(uint32_t frameNumber);

/** Frees all frames for a particular pid.
 * \param pid The pid associated with the frames we are want freed.
 * \param pageFrameMap The pointer to the beginning of the map.
 */
void freeAllFrames(uint32_t pid, uint8_t *pageFrameMap);

/** Counts the number of frames used by a particular process. Returns the number of frames used by that pid.
 * \param pid The pid you are interested in.
 * \param pageFrameMap The pointer to the beginning of the map.
 */
uint32_t processFramesUsed(uint32_t pid, uint8_t *pageFrameMap);

/** Counts the total frames used in the system. Returns that value.
 * \param pageFrameMap The pointer to the beginning of the map.
 */
uint32_t totalFramesUsed(uint8_t *pageFrameMap);