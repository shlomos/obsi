#include <click/config.h>
#include "stringmatcher.hh"
#include <click/glue.hh>
#include <click/error.hh>
#include <click/args.hh>
#include <click/confparse.hh>
#include <click/router.hh>
CLICK_DECLS

StringMatcher::StringMatcher() :
        _matches(0), _matcher(NULL)
{
}

StringMatcher::~StringMatcher()
{
    if (_matcher)
        delete _matcher;
}

int StringMatcher::configure(Vector<String> &conf, ErrorHandler *errh)
{
    String m_type; 
    const char *matcher_type;

    if (Args(this, errh).bind(conf)
        .read_m("MATCHER", m_type)
        .consume() < 0)
      return -1;

    matcher_type = m_type.c_str();
    if (!strncmp(matcher_type, AhoCorasick::NAME, strlen(AhoCorasick::NAME))) {
        _matcher = new AhoCorasick();
    } else if (!strncmp(matcher_type, WuManber::NAME, strlen(WuManber::NAME))) {
        _matcher = new WuManber();
    } else {
        errh->error("Unknown matcher type %s", matcher_type);
        return -1;
    }

    // This check will prevent us from doing any changes to the state if there is an error
    if (!is_valid_patterns(conf, errh)) {
        return -1; 
    }

    // if we are reconfiguring we need to reset the patterns and the matcher
    if ((!_matcher->is_open()) || _patterns.size()) {
        _matcher->reset();
        _patterns.clear();
    }

    for (int i = 0; i < conf.size(); ++i) {
        // All patterns should be OK so we can only have duplicates 
        if (_matcher->add_pattern(cp_unquote(conf[i]), i)) {
            errh->warning("Pattern #%d is a duplicate", i);
        } else {
            _patterns.push_back(conf[i]);        
        }
    }

    _matcher->finalize();


    if (!errh->nerrors()) {
        return 0;
    } else {
        return -1;
    }
}

bool StringMatcher::is_valid_patterns(Vector<String> &patterns, ErrorHandler *errh) const {
    bool valid = true;
    MyMatcher *matcher;

    if (!strncmp(_matcher->getMatcherType(), AhoCorasick::NAME, strlen(AhoCorasick::NAME))) {
        matcher = new AhoCorasick();
    } else if (!strncmp(_matcher->getMatcherType(), WuManber::NAME, strlen(WuManber::NAME))) {
        matcher = new WuManber();
    } else {
        valid = false;
        errh->error("Unknown matcher type %s", _matcher->getMatcherType());
        goto out;
    }

    for (int i = 0; i < patterns.size(); ++i) {
        MyMatcher::EnumReturnStatus rv = matcher->add_pattern(patterns[i], i);
        switch (rv) {
            case MyMatcher::MATCHER_ERROR_ZERO_PATTERN:
                errh->error("Pattern #%d has zero length", i);
                valid = false;
                break;
            case MyMatcher::MATCHER_ERROR_LONG_PATTERN:
                errh->error("Pattern #%d is too long", i);
                valid = false;
                break;
            case MyMatcher::MATCHER_ERROR_GENERAL:
                errh->error("Pattern #%d had unknown error", i);
                valid = false;
                break;
            case MyMatcher::MATCHER_TOO_MANY_PATTERNS:
                errh->error("Too many patterns added");
                valid = false;
                break;
            default:
                break;
        }
    }

    delete matcher;
out:
    return valid;
}

Packet* StringMatcher::simple_action(Packet *p) {
    if (_matcher->match_any(p)) {
        _matches++;

        // push to port 1 if anything is connected
        if (noutputs() == 2) {
            output(1).push(p);
        } else {
            p->kill();
        }
        return 0;
    } else {
        // push to port 0 
        return p; 
    }
}

int
StringMatcher::write_handler(const String &, Element *e, void *, ErrorHandler *) {
    StringMatcher * string_matcher = static_cast<StringMatcher *>(e);
    string_matcher->_matches = 0;
    return 0;    
}

void StringMatcher::add_handlers() {
    add_data_handlers("matches", Handler::h_read, &_matches);
    add_write_handler("reset_count", write_handler, 0, Handler::h_button | Handler::h_nonexclusive);
}

CLICK_ENDDECLS
ELEMENT_REQUIRES(userlevel AhoCorasick)
ELEMENT_REQUIRES(userlevel WuManber)
EXPORT_ELEMENT(StringMatcher)
ELEMENT_MT_SAFE(StringMatcher)
