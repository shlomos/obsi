#ifndef CLICK_AHOCORASICK_OTHER_H_
#define CLICK_AHOCORASICK_OTHER_H_
#include "matcher.hh"
#include "cac/AhoCorasick/ACBuilder.h"
#include "cac/StateMachine/TableStateMachine.h"
#include <click/string.hh>
#include <click/packet.hh>
CLICK_DECLS

class AhoCorasick_Other : public MyMatcher
{
	public:
		static const char *NAME;
		AhoCorasick_Other();
		~AhoCorasick_Other();
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
		bool match_any (const char* text, int size);
		int match_first(const char* text, int size);
		TableStateMachine *_ps;
        ACTree *_tree;
		bool _is_open;
        /* fucked up shit */
        int num_patterns;
};

CLICK_ENDDECLS
#endif /* CLICK_AHOCORASICK_OTHER_H_ */
