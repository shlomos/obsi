#include <click/config.h>
#include <click/error.hh>
#include "modifiedwumanber.hh"
CLICK_DECLS

const char *WuManber::NAME = "wumanber";

WuManber::WuManber ()
{
	_ps = (MWM_STRUCT *)mwmNew();
	_is_open = false;
}

WuManber::~WuManber ()
{
	mwmFree(_ps);
}

MyMatcher::EnumReturnStatus
WuManber::add_pattern (const String &pattern, PatternId id)
{
	/*
	 * The snort example set IID to 3000 for some wierd reason,
	 * I guess its not used.
	 * We set it predetermined to case in-sensitive, might add
	 * an argument to change that.
	 */
	return (MyMatcher::EnumReturnStatus)mwmAddPatternEx(
			_ps, (unsigned char *)pattern.c_str(), pattern.length(),
			1, 0, 0, (void *)((intptr_t)id), 3000);

}

MyMatcher::EnumReturnStatus
WuManber::add_pattern(const char pattern[], PatternId id)
{
	String tmpString = pattern;
	return add_pattern(tmpString, id);
}

MyMatcher::EnumReturnStatus
WuManber::add_pattern(const char *pattern, size_t len, PatternId id)
{
    (MyMatcher::EnumReturnStatus)mwmAddPatternEx(
			_ps, (unsigned char *)pattern, len,
			1, 0, 0, (void *)((intptr_t)id), 3000);
}

bool WuManber::match_any(const String& text) {
	return match_any(text.c_str(), text.length());
}

bool WuManber::match_any(const Packet* p) {
	return match_any((char *)p->data(), p->length());
}

int WuManber::match_first(const Packet* p) {
	return match_first((char *)p->data(), p->length());
}

int WuManber::match_first(const String& text) {
	return match_first(text.c_str(), text.length());
}

void WuManber::finalize()
{
	mwmLargeShifts(_ps, 1);
	mwmPrepPatterns(_ps);
	_is_open = true;
}

struct mwm_match{
	int patt_id;
} mwm_match;

int match(void *id, int index, void *data)
{
	(void)(index);
	struct mwm_match *match = (struct mwm_match *)data;
	match->patt_id = (int)((intptr_t)id);
	return 0;
}

bool WuManber::match_any(const char *text_to_match, int length) {
	int num_matches;

	num_matches = mwmSearch((void *)_ps, (unsigned char *)text_to_match, length,
			match, &mwm_match); 
	return num_matches > 0;
}


int WuManber::match_first(const char *text_to_match, int length) {
	int num_matches;

	num_matches = mwmSearch((void *)_ps, (unsigned char *)text_to_match, length,
			match, &mwm_match); 
	if (num_matches > 0) {
		return mwm_match.patt_id;
	} 
	return -1;
}

void WuManber::reset() {
	mwmFree(_ps);
	_is_open = false;
	_ps = (MWM_STRUCT *)mwmNew();
}

bool WuManber::is_open() {
	return _is_open;
}

CLICK_ENDDECLS
ELEMENT_REQUIRES(userlevel WuManberC)
ELEMENT_PROVIDES(WuManber)
ELEMENT_MT_SAFE(WuManber)
