#include "mytunnel.h"

using namespace cliptund;


Tunnel::~Tunnel()
{
	delete this->pump_cl2srv.cnn_src;
	delete this->pump_cl2srv.cnn_dst;
}

Pump::Pump() : eof(0)
{
	rfifo_init(&this->buf);
}

Tunnel::Tunnel(Connection *cnn_cl)
{
	cnn_cl->tun = this;
	pump_cl2srv.cnn_src = cnn_cl;
	pump_srv2cl.cnn_dst = cnn_cl;
}

void Tunnel::connected()
{
	Connection *cnn_cl = pump_cl2srv.cnn_src;
	Connection  *cnn_srv = pump_cl2srv.cnn_dst;

	cnn_cl->pump_recv = &pump_srv2cl;
	cnn_srv->pump_send = &pump_srv2cl;

	cnn_cl->pump_send = &pump_cl2srv;
	cnn_srv->pump_recv = &pump_cl2srv;

	pump_cl2srv.bufferavail();
	pump_srv2cl.bufferavail();
}

void Pump::bufferavail()
{
	this->cnn_src->bufferavail();
}

void Pump::havedata()
{
	this->cnn_dst->havedata();
}

#pragma warning ( disable : 4355 )
Connection::Connection(Tunnel *_tun) : tun(_tun ? _tun : new Tunnel(this)) {
	if (_tun) {
		_tun->pump_cl2srv.cnn_dst = this;
		_tun->pump_srv2cl.cnn_src = this;
	}
}
