#ifndef _MYTUNNEL_H
#define _MYTUNNEL_H

#include "myeventloop.h"
#include "rfifo.h"

//#include <atlbase.h>

namespace cliptund {

struct Connection;

struct Pump {
	rfifo_t buf;
	Connection *cnn_src;
	Connection *cnn_dst;
	Pump();
	void bufferavail();
	void havedata();
};

struct Tunnel : SimpleRefcount {

	Tunnel(Connection  *cnn_cl);
	void connected(Connection  *cnn_srv);
private:
	Pump pump_cl2srv;
	Pump pump_srv2cl;
	~Tunnel();
};

struct Connection {
	Tunnel *tun;
	Connection(Tunnel *_tun);

	Pump *pump_src;
	Pump *pump_dst;
	virtual void recv() = 0;
	virtual void send() = 0;
	virtual ~Connection() {};
};

struct ConnectionFactory {
	virtual void connect(Tunnel *tun) = 0;
};

}

#endif
