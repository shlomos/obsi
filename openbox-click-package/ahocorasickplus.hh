/*
 *
 * This file is part of multifast. Modified to work with click by Pavel Lazar
 *
	Copyright 2010-2015 Kamiar Kanani <kamiar.kanani@gmail.com>

	multifast is free software: you can redistribute it and/or modify
	it under the terms of the GNU Lesser General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	multifast is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public License
	along with multifast.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef CLICK_AHOCORASICKPLUS_H_
#define CLICK_AHOCORASICKPLUS_H_
#include "matcher.hh"
#include "ac/ahocorasick.h"
#include <click/string.hh>
#include <click/packet.hh>
CLICK_DECLS

// Forward declarations
struct ac_trie;
struct ac_text;


class AhoCorasick : public MyMatcher
{
	public:
		static const char *NAME;
		AhoCorasick();
		~AhoCorasick();
		const char *getMatcherType() const override { return NAME; }
		EnumReturnStatus add_pattern(const String &pattern, PatternId id);
		EnumReturnStatus add_pattern(const char pattern[], PatternId id);
		EnumReturnStatus add_pattern(const char *pattern, size_t len, PatternId id);
		void finalize();
		void reset(); 
		bool match_any (const String& text);
		bool match_any (const Packet *p);
		int match_first(const Packet *p);
		int match_first(const String& text);
		bool is_open();
		
	private:
		bool match_any(const char* text, int size, bool keep);
		int match_first(const char* text, int size, bool keep);
		struct ac_trie      *_automata;
		AC_TEXT_t *_text; 
};

CLICK_ENDDECLS
#endif /* CLICK_AHOCORASICKPLUS_H_ */
