package com.common.jsp.beans;

import java.util.Map;

import com.common.test.v24.EspaBus;
import com.common.test.v24.EtherSerialAdapter;
import com.common.test.v24.EtherSerialPort;
import com.common.test.v24.GtwayV24Data;
import com.common.test.v24.V24Device;

public class TestService {
	
	public static final String IP_ADDR = "192.168.1.2";
	public static final Integer TCP_PORT = 950;
	public static final String BUS_ADDR = "9";

	@SuppressWarnings("deprecation")
	public static GtwayV24Data getGtwayV24Data() {
		GtwayV24Data rootObj = new GtwayV24Data();
		Map<String, EtherSerialAdapter> adaptersByIpStr = rootObj.getAdaptersByIpStr();
		EtherSerialAdapter adapter = new EtherSerialAdapter();
		adapter.putToMap(adaptersByIpStr, IP_ADDR);
		// adaptersByIpStr.put(IP_ADDR, adapter);
		adapter.setName("test adapter");
		Map<Integer, EtherSerialPort> serialsByTcpServerPort = adapter.getSerialsByTcpServerPort();
		EtherSerialPort portObj = new EtherSerialPort();
		serialsByTcpServerPort.put(TCP_PORT, portObj);
		portObj.setNportCommandPort(966);
		EspaBus bus = new EspaBus();
		portObj.setBus(bus);
		bus.setBaudRate(9600);
		bus.setOurAddress('2');
		V24Device endpoint = new V24Device();
		bus.getEndpointsByBusAddr().put(BUS_ADDR, endpoint);
		endpoint.setV24DeviceId(1001);
		return rootObj;
	}

}
