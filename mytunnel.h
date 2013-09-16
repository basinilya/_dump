#ifndef _MYTUNNEL_H
#define _MYTUNNEL_H

#include "cliptund.h"
#include "myeventloop.h"
#include "rfifo.h"

//#include <atlbase.h>

namespace cliptund {

struct Connection;

struct Pump {
	rfifo_t buf;
	int eof;
	Connection *cnn_src;
	Connection *cnn_dst;
	Pump();
	void havedata();
	void bufferavail();
};

struct Tunnel : SimpleRefcount {

	Tunnel(Connection  *cnn_cl);
	void connected();
//private:
	Pump pump_cl2srv;
	Pump pump_srv2cl;
	~Tunnel();
};

struct Connection {
	Tunnel *tun;
	Connection(Tunnel *_tun);

	Pump *pump_recv;
	Pump *pump_send;
	virtual void bufferavail() = 0;
	virtual void havedata() = 0;
	virtual ~Connection() {};
};

struct ConnectionFactory {
	virtual void connect(Tunnel *tun) = 0;
};

}

#endif
