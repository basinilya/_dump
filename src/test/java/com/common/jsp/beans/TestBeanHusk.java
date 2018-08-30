package com.common.jsp.beans;

import java.util.Map;

import com.common.test.v24.EspaBus;
import com.common.test.v24.EtherSerialAdapter;
import com.common.test.v24.EtherSerialPort;
import com.common.test.v24.GtwayV24Data;
import com.common.test.v24.V24Device;

import junit.framework.TestCase;

public class TestBeanHusk extends TestCase {

	public void testBeanHusk() throws Exception {
		BeanHusk rootHusk = new BeanHusk();
		rootHusk.setValue(rootObj);
		BeanHusk huskNportSettings = rootHusk.getProperties().get("nportSettings");
		assertEquals(true, huskNportSettings.getProperties().get("discovery").getValue());
		BeanHusk huskAdaptersByIpStr = rootHusk.getProperties().get("adaptersByIpStr");

		BeanHusk huskAdapter = huskAdaptersByIpStr.getProperties().get("0");
		assertEquals("192.168.1.2", huskAdapter.getDisplayName());

		assertEquals("test adapter", huskAdapter.getProperties().get("name").getValue());
		rootObj.getAdaptersByIpStr().get("1").getSerialsByTcpServerPort().get("1").getBus();
		BeanHusk huskSerialsByTcpServerPort = huskAdapter.getProperties().get("serialsByTcpServerPort");

		BeanHusk huskPortObj = huskSerialsByTcpServerPort.getProperties().get("0");
		assertEquals(950, huskPortObj.getDisplayName());
		assertEquals(966, huskPortObj.getProperties().get("nportCommandPort").getValue());

		BeanHusk huskBus = huskPortObj.getProperties().get("bus");
		assertEquals(9600, huskBus.getProperties().get("baudRate").getValue());
		assertEquals('2', huskBus.getProperties().get("ourAddress").getValue());

		BeanHusk huskEndpointsByBusAddr = huskBus.getProperties().get("endpointsByBusAddr");
		BeanHusk huskEndpoint = huskEndpointsByBusAddr.getProperties().get("0");
		assertEquals("9", huskEndpoint.getDisplayName());
		assertEquals(1001, huskEndpoint.getProperties().get("v24DeviceId").getValue());
	}

	private GtwayV24Data rootObj;

	@SuppressWarnings("deprecation")
	@Override
	protected void setUp() throws Exception {
		super.setUp();
		rootObj = new GtwayV24Data();
		Map<String, EtherSerialAdapter> adaptersByIpStr = rootObj.getAdaptersByIpStr();
		EtherSerialAdapter adapter = new EtherSerialAdapter();
		adapter.putToMap(adaptersByIpStr, "192.168.1.2");
		// adaptersByIpStr.put("192.168.1.2", adapter);
		adapter.setName("test adapter");
		Map<Integer, EtherSerialPort> serialsByTcpServerPort = adapter.getSerialsByTcpServerPort();
		EtherSerialPort portObj = new EtherSerialPort();
		serialsByTcpServerPort.put(950, portObj);
		portObj.setNportCommandPort(966);
		EspaBus bus = new EspaBus();
		portObj.setBus(bus);
		bus.setBaudRate(9600);
		bus.setOurAddress('2');
		V24Device endpoint = new V24Device();
		bus.getEndpointsByBusAddr().put("9", endpoint);
		endpoint.setV24DeviceId(1001);
	}

}
