#pragma once

// 2 1st bytes = header
// first 4 bits = message type
// 5th bit = DUP flag
// 6th and 7th bit - QoS Level
// 8th bit RETAIN flag
// 2nd byte = Remaining length, 1-4 bytes
