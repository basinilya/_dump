#include "mytunnel.h"

using namespace cliptund;


Tunnel::~Tunnel()
{
	delete this->pump_cl2srv.cnn_src;
	delete this->pump_cl2srv.cnn_dst;
}

Pump::Pump()
{
	rfifo_init(&this->buf);
}

Tunnel::Tunnel(Connection  *cnn_cl)
{
	pump_cl2srv.cnn_src = cnn_cl;
	pump_srv2cl.cnn_dst = cnn_cl;
	cnn_cl->tun = this;
}

void Tunnel::connected(Connection  *cnn_srv)
{
	Connection *cnn_cl = pump_cl2srv.cnn_src;

	pump_cl2srv.cnn_dst = cnn_srv;
	pump_srv2cl.cnn_src = cnn_srv;

	cnn_cl->pump_src = &pump_srv2cl;
	cnn_srv->pump_dst = &pump_srv2cl;

	cnn_cl->pump_dst = &pump_cl2srv;
	cnn_srv->pump_src = &pump_cl2srv;

	pump_cl2srv.bufferavail();
	pump_srv2cl.bufferavail();
}

void Pump::bufferavail()
{
	this->cnn_src->recv();
}

void Pump::havedata()
{
	this->cnn_dst->send();
}
