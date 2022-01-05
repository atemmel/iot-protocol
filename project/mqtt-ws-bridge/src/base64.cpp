#include "base64.hpp"

#include <bitset>
#include <iostream>

namespace base64 {

constexpr std::string_view table =
"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
"abcdefghijklmnopqrstuvwxyz"
"0123456789+/=";

auto encode(std::string_view view) -> std::string {
	std::string result;
	result.reserve(view.size() * 2);

	// 8 * 3 = 24
	// 24 % 6 == 0
	for(size_t step = 0; step < view.size(); step += 3) {
		char c0, c1, c2, c3;

		c0 = (view[step] & 0xFC) >> 2;

		if(step + 1 < view.size()) {
			c1 = ((view[step] & 0x03) << 4) + ((view[step + 1] & 0xF0) >> 4);
			if(step + 2 < view.size()) {
				c2 = ((view[step + 1] & 0x0F) << 2) + ((view[step + 2] & 0xC0) >> 6);
				c3 = (view[step + 2] & 0x3F);
			} else {
				c2 = (view[step + 1] & 0x0f) << 2;
				c3 = 64;
			}
		} else {
			c1 = (view[step] & 0x03) << 4;
			c2 = 64;
			c3 = 64;
		}

		result.push_back(table[c0]);
		result.push_back(table[c1]);
		result.push_back(table[c2]);
		result.push_back(table[c3]);
	}

	return result;
}

};
