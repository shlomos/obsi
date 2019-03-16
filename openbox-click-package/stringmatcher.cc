#include <click/config.h>
#include "stringmatcher.hh"
#include <click/glue.hh>
#include <click/error.hh>
#include <click/args.hh>
#include <click/confparse.hh>
#include <click/router.hh>

#include <execinfo.h>
#include <stdio.h>
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
    char bla[4096] = {0};

    if (Args(this, errh).bind(conf)
        .read_m("MATCHER", m_type)
        .consume() < 0)
      return -1;

    matcher_type = m_type.c_str();
    if (!strncmp(matcher_type, AhoCorasick::NAME, strlen(AhoCorasick::NAME))) {
        _matcher = new AhoCorasick();
    } else if (!strncmp(matcher_type, WuManber::NAME, strlen(WuManber::NAME))) {
        _matcher = new WuManber();
    } else if (!strncmp(matcher_type, CompressedAhoCorasick::NAME, strlen(CompressedAhoCorasick::NAME))) {
        _matcher = new CompressedAhoCorasick();
    } else {
        errh->error("Unknown matcher type %s", matcher_type);
        return -1;
    }

    // This check will prevent us from doing any changes to the state if there is an error
    if (!is_valid_patterns(conf, errh)) {
        printf("invalid patterns!\n");
        return -1; 
    }

    // if we are reconfiguring we need to reset the patterns and the matcher
    if ((!_matcher->is_open()) || _patterns.size()) {
        _matcher->reset();
        _patterns.clear();
    }

    for (int i = 0; i < conf.size(); ++i) {
        // All patterns should be OK so we can only have duplicates
        //printf("patt %d = -- %s -- \n", i, cp_unquote(conf[i]).c_str());
        //snprintf(bla, 4096, "echo \"patt %d = -- %s -- \" >> /tmp/ga_out.txt", i, cp_unquote(conf[i]).c_str());
        //system(bla);
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
    } else if (!strncmp(_matcher->getMatcherType(), CompressedAhoCorasick::NAME, strlen(CompressedAhoCorasick::NAME))) {
        matcher = new CompressedAhoCorasick();
    } else {
        valid = false;
        errh->error("Invalid matcher type %s", _matcher->getMatcherType());
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

inline int
StringMatcher::classify(Packet *p) {
    if (_matcher->match_any(p)) {
        _matches++;
        return 1;
    }
    return 0;
}


Packet* StringMatcher::simple_action(Packet *p) {
    int port = classify(p);

    if (port > noutputs() - 1 || port < 0){
        p->kill();
        goto done;
    }
    output(port).push(p);

done:
    return 0;
}

//#if HAVE_BATCH
// void StringMatcher::push_batch(int, PacketBatch *batch) {
//       FOR_EACH_PACKET_SAFE(batch, p) {
//             durak(p, 'a');
//       }
// }
//#endif /* HAVE_BATCH */

// void StringMatcher::push(int, Packet *p) {
//     void* callstack[128];
//      int i, frames = backtrace(callstack, 128);
//      char** strs = backtrace_symbols(callstack, frames);
//      for (i = 0; i < frames; ++i) {
//          printf("%s\n", strs[i]);
//      }
//      free(strs);
//     int port = classify(p);
//     printf("sa: %d\n", port);
//     if (port > noutputs() - 1 || port < 0){
//         p->kill();
//         return;
//     }
//     output(port).push(p);
// }

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
ELEMENT_REQUIRES(userlevel CompressedAhoCorasick)
EXPORT_ELEMENT(StringMatcher)
ELEMENT_MT_SAFE(StringMatcher)
