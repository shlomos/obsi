#include <float.h>
#include <click/config.h>
#include <click/args.hh>
#include <click/glue.hh>
#include <click/error.hh>
#include <click/router.hh>
#include <elements/json/json.hh>
#include "utilizationmonitor.hh"
CLICK_DECLS

UtilizationMonitor::UtilizationMonitor():
		_usec_accum(0), _count(0), _windows_counts(1), _avg_window_proc(0), _last_check(DBL_MAX)
{
}

UtilizationMonitor::~UtilizationMonitor()
{
}

void UtilizationMonitor::format_message() {
	Json message = Json();
	char cont[256] = {0};
	snprintf(cont, sizeof(cont), "{\"block\": \"%s\"}", _protected_block.c_str());
	message.set("type", "CASTLE");
	message.set("content", cont);
	_formated_message = message.unparse();
}

int
UtilizationMonitor::configure(Vector<String> &conf, ErrorHandler *errh)
{
	if (Args(conf, this, errh)
			.read_mp("WINDOW", _window_size)
			.read_mp("PROC_THRESHOLD", _thresh)
			.read_mp("BLOCK", StringArg(), _protected_block)
			.complete() < 0)
		return -1;
	_ehandler = router()->chatter_channel("openbox");
	format_message();
	return 0;
}

UtilizationMonitor *
UtilizationMonitor::hotswap_element() const
{
	if (Element *e = Element::hotswap_element())
		if (UtilizationMonitor *um = static_cast<UtilizationMonitor *>(e->cast("UtilizationMonitor")))
			if (um->_protected_block == _protected_block)
				return um;
	return 0;
}

void
UtilizationMonitor::take_state(Element *e, ErrorHandler *errh)
{
	(void)errh;
	UtilizationMonitor *o = static_cast<UtilizationMonitor *>(e); // checked by hotswap_element()

	_usec_accum = 0;
	_count = 0;
	_windows_counts = o->_windows_counts;
	_avg_window_proc = o->_avg_window_proc;
	_last_check = DBL_MAX;
}

void UtilizationMonitor::emit_alert() const {
	_ehandler->message(_formated_message.c_str());
}

void UtilizationMonitor::analyze(Packet *p) {
	double avg_proc = 0;

	_usec_accum += (Timestamp::now() - p->timestamp_anno()).doubleval();
	_count++;

	if (_count >= _window_size || Timestamp::now().doubleval() - _last_check > 5.0) {
		_last_check = Timestamp::now().doubleval();
		avg_proc = _usec_accum / _count;
		printf("avg_proc = %f, _thresh * _avg_window_proc = %f, _windows_counts=%d\n", avg_proc, _thresh * _avg_window_proc, _windows_counts);
		if ((avg_proc > _thresh * _avg_window_proc) && _windows_counts > 2) {
			emit_alert();
		} else {
			_avg_window_proc = ((_windows_counts - 1) * _avg_window_proc + avg_proc) / _windows_counts;
			_windows_counts++;
		}
		_count = 0;
		_usec_accum = 0;
	}

}

void UtilizationMonitor::push(int port, Packet *p)
{
	analyze(p);
	output(port).push(p);
}

#if HAVE_BATCH
void UtilizationMonitor::push_batch(int port, PacketBatch *batch) {
	FOR_EACH_PACKET(batch, p)
		analyze(p);
	output(port).push_batch(batch);
}
#endif

String UtilizationMonitor::read_handler(Element *e, void *thunk)
{
	UtilizationMonitor *ta = static_cast<UtilizationMonitor *>(e);
	int which = reinterpret_cast<intptr_t>(thunk);
	switch (which) {
	case 0:
		return String(ta->_count);
	case 1:
		return String(ta->_usec_accum);
	case 2:
		return String(ta->_usec_accum / ta->_count);
	case 3:
		return String(ta->_window_size);
	case 4:
		return String(ta->_avg_window_proc);
	case 5:
		return String(ta->_thresh);
	default:
		return String();
	}
}

int
UtilizationMonitor::reset_handler(const String &, Element *e, void *, ErrorHandler *)
{
	UtilizationMonitor *ta = static_cast<UtilizationMonitor *>(e);
	ta->_usec_accum = 0;
	ta->_count = 0;
	ta->_windows_counts = 1;
	ta->_avg_window_proc = 0;
	return 0;
}

void
UtilizationMonitor::add_handlers()
{
	add_read_handler("count", read_handler, 0);
	add_read_handler("time", read_handler, 1);
	add_read_handler("average_time", read_handler, 2);
	add_read_handler("window", read_handler, 3);
	add_read_handler("average_window_time", read_handler, 4);
	add_read_handler("proc_threshold", read_handler, 5);
	add_write_handler("reset_counts", reset_handler, 0, Handler::f_button);
}

CLICK_ENDDECLS
EXPORT_ELEMENT(UtilizationMonitor)
