#ifndef _MYTUNNEL_H
#define _MYTUNNEL_H

#include "cliptund.h"
#include "myeventloop.h"
#include "rfifo.h"

//#include <atlbase.h>

namespace cliptund {

struct Connection;
struct Tunnel;

struct Pump {
	rfifo_t buf;
	int eof;
	int writeerror;
	Connection *cnn_src;
	Connection *cnn_dst;
	Pump();
	void havedata();
	void bufferavail();
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

struct Tunnel : SimpleRefcount {
	void connected();
	Pump pump_cl2srv;
	Pump pump_srv2cl;
	~Tunnel();
	friend Connection::Connection(Tunnel *);
private:
	Tunnel(Connection  *cnn_cl);
};

struct ConnectionFactory {
	virtual void connect(Tunnel *tun) = 0;
};

}

#endif
