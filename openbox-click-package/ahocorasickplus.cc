/*
 * ahocorasickplus.cc: A sample C++ wrapper for Aho-Corasick C library 
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
#include <click/config.h>
#include "ahocorasickplus.hh"
CLICK_DECLS

const char *AhoCorasick::NAME = "ahocorasick";

AhoCorasick::AhoCorasick()
{
	_automata = ac_trie_create ();
	_text = new AC_TEXT_t;
}

AhoCorasick::~AhoCorasick()
{
	ac_trie_release (_automata);
	delete _text;
}

MyMatcher::EnumReturnStatus
AhoCorasick::add_pattern(const String &pattern, PatternId id)
{
	// Adds zero-terminating string

	EnumReturnStatus rv = MATCHER_ERROR_GENERAL;

	AC_PATTERN_t patt;
	patt.ptext.astring = (AC_ALPHABET_t*) pattern.c_str();
	patt.ptext.length = pattern.length();
	patt.id.u.number = id;
	patt.rtext.astring = NULL;
	patt.rtext.length = 0;

	AC_STATUS_t status = ac_trie_add (_automata, &patt, 0);

	switch (status)
	{
		case ACERR_SUCCESS:
			rv = MATCHER_SUCCESS;
			break;
		case ACERR_DUPLICATE_PATTERN:
			rv = MATCHER_ERROR_DUPLICATE_PATTERN;
			break;
		case ACERR_LONG_PATTERN:
			rv = MATCHER_ERROR_LONG_PATTERN;
			break;
		case ACERR_ZERO_PATTERN:
			rv = MATCHER_ERROR_ZERO_PATTERN;
			break;
		case ACERR_TRIE_CLOSED:
			rv = MATCHER_ERROR_CLOSED;
			break;
	}
	return rv;
}

MyMatcher::EnumReturnStatus
AhoCorasick::add_pattern (const char pattern[], PatternId id)
{
	String tmpString = pattern;
	return add_pattern (tmpString, id);
}

MyMatcher::EnumReturnStatus
AhoCorasick::add_pattern(const char *pattern, size_t len, PatternId id)
{
    EnumReturnStatus rv = MATCHER_ERROR_GENERAL;

	AC_PATTERN_t patt;
	patt.ptext.astring = (AC_ALPHABET_t*)pattern;
	patt.ptext.length = len;
	patt.id.u.number = id;
	patt.rtext.astring = NULL;
	patt.rtext.length = 0;

	AC_STATUS_t status = ac_trie_add (_automata, &patt, 0);

	switch (status)
	{
		case ACERR_SUCCESS:
			rv = MATCHER_SUCCESS;
			break;
		case ACERR_DUPLICATE_PATTERN:
			rv = MATCHER_ERROR_DUPLICATE_PATTERN;
			break;
		case ACERR_LONG_PATTERN:
			rv = MATCHER_ERROR_LONG_PATTERN;
			break;
		case ACERR_ZERO_PATTERN:
			rv = MATCHER_ERROR_ZERO_PATTERN;
			break;
		case ACERR_TRIE_CLOSED:
			rv = MATCHER_ERROR_CLOSED;
			break;
	}
	return rv;
}

bool AhoCorasick::match_any(const String& text)
{
	return match_any(text.c_str(), text.length(), false);
}

bool AhoCorasick::match_any(const Packet* p)
{
	return match_any((char *)p->data(), p->length(), false);
}

int AhoCorasick::match_first(const Packet* p)
{
	return match_first((char *)p->data(), p->length(), false);
}

int AhoCorasick::match_first(const String& text)
{
	return match_first(text.c_str(), text.length(), false);
}

void AhoCorasick::finalize()
{
	ac_trie_finalize (_automata);
}

bool AhoCorasick::match_any(const char* text_to_match, int length, bool keep)
{
	_text->astring = text_to_match;
	_text->length = length;
	ac_trie_settext(_automata, _text, (int)keep);
	AC_MATCH_t match = ac_trie_findnext(_automata);
	return match.size > 0;
}

int AhoCorasick::match_first(const char* text_to_match, int length, bool keep)
{
	_text->astring = text_to_match;
	_text->length = length;
	ac_trie_settext(_automata, _text, (int)keep);
	AC_MATCH_t match = ac_trie_findnext(_automata);
	if (match.size > 0) {
		return (int) match.patterns->id.u.number;
	} 
	return -1;
}

void AhoCorasick::reset()
{
	ac_trie_release (_automata);
	_automata = ac_trie_create ();
}

bool AhoCorasick::is_open()
{
	return _automata->trie_open != 0;
}

CLICK_ENDDECLS
ELEMENT_REQUIRES(userlevel AhoCorasickC)
ELEMENT_PROVIDES(AhoCorasick)
ELEMENT_MT_SAFE(AhoCorasick)
