#ifndef CLICK_UTILIZATIONMONITOR_HH
#define CLICK_UTILIZaTIONMONITOR_HH
#include <click/element.hh>
CLICK_DECLS

class UtilizationMonitor : public Element {

public:
	UtilizationMonitor() CLICK_COLD;
	~UtilizationMonitor() CLICK_COLD;

	const char *class_name() const	{ return "UtilizationMonitor"; }
	const char *port_count() const	{ return "1-/="; }
	const char *processing() const	{ return PUSH; }
	const char *flags() const	{ return "#/#"; }

	int configure(Vector<String> &, ErrorHandler *) CLICK_COLD;
	void add_handlers() CLICK_COLD;

	void push(int, Packet *);

 private:
	double _usec_accum;
	double _avg_window_proc;
	double _thresh;
	uint64_t _window_size;
	uint64_t _count;
	uint64_t _windows_counts;
	ErrorHandler *_ehandler; 
	String _formated_message;
	String _protected_block;

	void emit_alert() const; 
	void format_message(); 
	static String read_handler(Element *, void *) CLICK_COLD;
	static int reset_handler(const String &, Element *, void *, ErrorHandler *);

};

CLICK_ENDDECLS
#endif