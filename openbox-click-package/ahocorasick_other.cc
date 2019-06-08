#include <unistd.h>
#include <click/config.h>
#include <click/error.hh>
#include "ahocorasick_other.hh"
CLICK_DECLS

#define DEFAULT_COMMON_NUM 512
#define DEFAULT_UNCOMMON_LIM 30

const char *AhoCorasick_Other::NAME = "ahocorasick_other";

AhoCorasick_Other::AhoCorasick_Other()
{
	_tree = (ACTree *)calloc(1, sizeof(ACTree));
	_is_open = false;
	num_patterns = 0;
	_ps = NULL;
}

AhoCorasick_Other::~AhoCorasick_Other()
{
	if (_ps) {
		destroyTableStateMachine(_ps);
	}
	_ps = NULL;
}

MyMatcher::EnumReturnStatus
AhoCorasick_Other::add_pattern(const String &pattern, PatternId id)
{
	num_patterns++;
	return (MyMatcher::EnumReturnStatus)acAddPatternToTree(
			_tree, (unsigned char *)pattern.c_str(), pattern.length());
}

MyMatcher::EnumReturnStatus
AhoCorasick_Other::add_pattern(const char pattern[], PatternId id)
{
	String tmpString = pattern;
	return add_pattern(tmpString, id);
}

MyMatcher::EnumReturnStatus
AhoCorasick_Other::add_pattern(const char *pattern, size_t len, PatternId id)
{
	num_patterns++;
	return (MyMatcher::EnumReturnStatus)acAddPatternToTree(
			_tree, (unsigned char *)pattern, len);
}

bool AhoCorasick_Other::match_any(const String& text) {
	return match_any(text.c_str(), text.length());
}

bool AhoCorasick_Other::match_any(const Packet* p) {
	return match_any((char *)p->data(), p->length());
}

int AhoCorasick_Other::match_first(const Packet* p) {
	return match_first((char *)p->data(), p->length());
}

int AhoCorasick_Other::match_first(const String& text) {
	return match_first(text.c_str(), text.length());
}

void AhoCorasick_Other::finalize()
{
	/*
	 * Fucked up shit, i'm too lazy to fix now... 
	 * However it cost me a whole day and a brain damage to find out it happens, 
	 * so I write it here so you don't have to.
	 */
	if (num_patterns < 2) {
		fprintf(stderr, "This matcher does not work with only one pattern...\n");
		exit(-1);
	}
	/* end of fucked up shit */

	_ps = createTableStateMachine(_tree, DEFAULT_COMMON_NUM, (double)DEFAULT_UNCOMMON_LIM / 100.0);
	if (_tree)
		free(_tree);
	_tree = NULL;
	_is_open = true;
}

bool AhoCorasick_Other::match_any(const char *text_to_match, int length) {
	int num_matches, is_heavy, last_idx_in_root;
	double uncommonRate;
	char *text_cpy = (char *)calloc(length, sizeof(char));

	memcpy(text_cpy, text_to_match, length);
	num_matches = matchTableMachine(_ps, NULL, FALSE, text_cpy, length, 0, NULL, NULL, NULL, NULL, &is_heavy, &last_idx_in_root, &uncommonRate);
	return num_matches > 0;
}

int AhoCorasick_Other::match_first(const char *text_to_match, int length) {
	/* Not implemented */
	return -1;
}

void AhoCorasick_Other::reset() {
	if (_ps) {
		destroyTableStateMachine(_ps);
	}
	_is_open = false;
	_ps = NULL;
	free(_tree);
	_tree = (ACTree *)calloc(1, sizeof(ACTree));
	num_patterns = 0;
}

bool AhoCorasick_Other::is_open() {
	return _is_open;
}

CLICK_ENDDECLS
ELEMENT_REQUIRES(userlevel TableStateMachineC)
ELEMENT_REQUIRES(userlevel ACBuilderC)
ELEMENT_PROVIDES(AhoCorasick_Other)
ELEMENT_MT_SAFE(AhoCorasick_Other)
