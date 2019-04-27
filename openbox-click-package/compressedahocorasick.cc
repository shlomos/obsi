#include <unistd.h>
#include <click/config.h>
#include <click/error.hh>
#include "compressedahocorasick.hh"
CLICK_DECLS

#define DEFAULT_MAX_GOTOS_BM 64
#define DEFAULT_MAX_GOTOS_LE 4

const char *CompressedAhoCorasick::NAME = "compressedahocorasick";

CompressedAhoCorasick::CompressedAhoCorasick()
{
	_tree = (ACTree *)calloc(1, sizeof(ACTree));
	_stats = (MachineStats *)calloc(1, sizeof(MachineStats));
	_stats->totalFailures = 0;
	_stats->totalGotos = 0;
	_is_open = false;
	num_patterns = 0;
	_ps = NULL;
}

CompressedAhoCorasick::~CompressedAhoCorasick()
{
	if (_ps) {
		destroyStateMachine(_ps);
	}
	_ps = NULL;
}

MyMatcher::EnumReturnStatus
CompressedAhoCorasick::add_pattern(const String &pattern, PatternId id)
{
	num_patterns++;
	return (MyMatcher::EnumReturnStatus)acAddPatternToTree(
			_tree, (unsigned char *)pattern.c_str(), pattern.length());
}

MyMatcher::EnumReturnStatus
CompressedAhoCorasick::add_pattern(const char pattern[], PatternId id)
{
	String tmpString = pattern;
	return add_pattern(tmpString, id);
}

MyMatcher::EnumReturnStatus
CompressedAhoCorasick::add_pattern(const char *pattern, size_t len, PatternId id)
{
	num_patterns++;
	return (MyMatcher::EnumReturnStatus)acAddPatternToTree(
			_tree, (unsigned char *)pattern, len);
}

bool CompressedAhoCorasick::match_any(const String& text) {
	return match_any(text.c_str(), text.length());
}

bool CompressedAhoCorasick::match_any(const Packet* p) {
	return match_any((char *)p->data(), p->length());
}

int CompressedAhoCorasick::match_first(const Packet* p) {
	return match_first((char *)p->data(), p->length());
}

int CompressedAhoCorasick::match_first(const String& text) {
	return match_first(text.c_str(), text.length());
}

void CompressedAhoCorasick::finalize()
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
	_ps = createStateMachine(_tree, DEFAULT_MAX_GOTOS_LE, DEFAULT_MAX_GOTOS_BM, 0);
	free(_tree);
	_tree = NULL;
	_is_open = true;
}

bool CompressedAhoCorasick::match_any(const char *text_to_match, int length) {
	int num_matches;
	char *text_cpy = (char *)calloc(length, sizeof(char));

	memcpy(text_cpy, text_to_match, length);
	num_matches = match(_ps, text_cpy, length, 0, _stats, 0, 0);
	free(text_cpy);
	return num_matches > 0;
}

int CompressedAhoCorasick::match_first(const char *text_to_match, int length) {
	/* Not implemented */
	return -1;
}

void CompressedAhoCorasick::reset() {
	if (_ps) {
		destroyStateMachine(_ps);
	}
	_is_open = false;
	_ps = NULL;
	free(_tree);
	free(_stats);
	_tree = (ACTree *)calloc(1, sizeof(ACTree));
	_stats = (MachineStats *)calloc(1, sizeof(MachineStats));
	_stats->totalFailures = 0;
	_stats->totalGotos = 0;
	num_patterns = 0;
}

bool CompressedAhoCorasick::is_open() {
	return _is_open;
}

CLICK_ENDDECLS
ELEMENT_REQUIRES(userlevel StateMachineGeneratorC)
ELEMENT_REQUIRES(userlevel ACBuilderC)
ELEMENT_PROVIDES(CompressedAhoCorasick)
ELEMENT_MT_SAFE(CompressedAhoCorasick)
