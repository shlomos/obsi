#ifndef CLICK_MODIFIEDWUMANBER_H_
#define CLICK_MODIFIEDWUMANBER_H_
#include "matcher.hh"
#include "mwm/mwm.h"
#include <click/string.hh>
#include <click/packet.hh>
CLICK_DECLS

class WuManber : public MyMatcher
{
	public:
		static const char *NAME;
		WuManber();
		~WuManber();
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
		MWM_STRUCT *_ps;
		bool _is_open;
};

CLICK_ENDDECLS
#endif /*CLICK_MODIFIEDWUMANBER_H_*/
