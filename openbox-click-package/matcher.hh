#ifndef CLICK_MATCHER_H_
#define CLICK_MATCHER_H_
#include "actypes.h"
//#include <click/string.hh>
//#include <click/packet.hh>

CLICK_DECLS

class Packet;
class String;

class MyMatcher
{
	public:
		virtual ~MyMatcher();
		typedef unsigned int PatternId;

		struct Match {
			unsigned int    position;
			PatternId       id;
		};

		enum EnumReturnStatus {
			MATCHER_SUCCESS = 0,       // No error occurred
			MATCHER_ERROR_GENERAL,            // General unknown failure
			MATCHER_ERROR_DUPLICATE_PATTERN, // Duplicate patterns
			MATCHER_ERROR_LONG_PATTERN,      // Long pattern
			MATCHER_ERROR_ZERO_PATTERN,      // Empty pattern (zero length)
			MATCHER_ERROR_CLOSED,   // Machine is closed
			MATCHER_TOO_MANY_PATTERNS,
		};

		virtual const char *getMatcherType() const = 0;

		virtual EnumReturnStatus add_pattern(const String &pattern, PatternId id) = 0;
		virtual EnumReturnStatus add_pattern(const char pattern[], PatternId id) = 0;
		virtual void finalize() = 0;
		virtual void reset() = 0; 
		
		virtual bool match_any (const String& text) = 0;
		virtual bool match_any (const Packet *p) = 0;
		virtual int match_first(const Packet *p) = 0;
		virtual int match_first(const String& text) = 0;
		virtual bool is_open() = 0;
};

CLICK_ENDDECLS

#endif /* CLICK_MATCHER_H_ */
