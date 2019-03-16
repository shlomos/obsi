#ifndef CLICK_UTILIZATIONMONITOR_HH
#define CLICK_UTILIZaTIONMONITOR_HH
#include <click/batchelement.hh>
CLICK_DECLS

class UtilizationMonitor : public BatchElement {

public:
	UtilizationMonitor() CLICK_COLD;
	~UtilizationMonitor() CLICK_COLD;

	const char *class_name() const	{ return "UtilizationMonitor"; }
	const char *port_count() const	{ return "1-/="; }
	const char *processing() const	{ return PUSH; }
	const char *flags() const	{ return "#/#"; }

	int configure(Vector<String> &, ErrorHandler *) CLICK_COLD;
	void add_handlers() CLICK_COLD;

	UtilizationMonitor *hotswap_element() const;
	void take_state(Element *e, ErrorHandler *errh);

	void push(int, Packet *);
#if HAVE_BATCH
	void push_batch(int, PacketBatch *);
#endif

 private:
	double _usec_accum;
	double _avg_window_proc;
	double _thresh;
	uint64_t _window_size;
	double _last_check;
	uint64_t _count;
	uint64_t _windows_counts;
	ErrorHandler *_ehandler; 
	String _formated_message;
	String _protected_block;

	void emit_alert() const; 
	void format_message();
	void analyze(Packet *);
	static String read_handler(Element *, void *) CLICK_COLD;
	static int reset_handler(const String &, Element *, void *, ErrorHandler *);

};

CLICK_ENDDECLS
#endif