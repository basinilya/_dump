#ifndef _MYTUNNEL_H
#define _MYTUNNEL_H

#include "myeventloop.h"
#include "rfifo.h"

//#include <atlbase.h>

namespace cliptund {

struct Tunnel;
struct Pump;

struct Connection {
	ISimpleRefcount *owner;
	Pump *pump_src;
	Pump *pump_dst;
	virtual void recv() = 0;
	virtual void send() = 0;
	virtual ~Connection() = 0;
};

struct Pump {
	rfifo_t buf;
	Connection *cnn_src;
	Connection *cnn_dst;
	Pump();
	void bufferavail();
	void havedata();
};

struct Tunnel : SimpleRefcount {

	Pump pump_cl2srv;
	Pump pump_srv2cl;

	Tunnel();
	void connected();
private:
	~Tunnel();
};

}


#endif
