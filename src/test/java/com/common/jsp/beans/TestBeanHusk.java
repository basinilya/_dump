package com.common.jsp.beans;

import java.beans.Introspector;
import java.lang.reflect.Method;
import java.lang.reflect.ParameterizedType;
import java.lang.reflect.Type;
import java.lang.reflect.TypeVariable;
import java.util.Arrays;
import java.util.Map;
import java.util.function.Function;

import org.mockito.Mockito;
import org.mockito.invocation.InvocationOnMock;

import com.common.test.v24.EspaBus;
import com.common.test.v24.EtherSerialAdapter;
import com.common.test.v24.EtherSerialPort;
import com.common.test.v24.GtwayV24Data;
import com.common.test.v24.NPortSettings;
import com.common.test.v24.SerialBus;
import com.common.test.v24.V24Device;
import com.google.common.reflect.TypeToken;

import junit.framework.TestCase;

public class TestBeanHusk extends TestCase {

	private static final String IP_ADDR = "192.168.1.2";
	private static final Integer TCP_PORT = 950;
	private static final String BUS_ADDR = "9";

	@SuppressWarnings("deprecation")
	public void testBeanHusk() throws Exception {
		BeanHusk rootHusk = new BeanHusk(rootObj);

		assertEquals(GtwayV24Data.class.getSimpleName(), rootHusk.getTypeName());
		
		BeanHusk huskNportSettings = getProp(rootHusk, GtwayV24Data::getNportSettings);
		assertEquals(NPortSettings.class.getSimpleName(), huskNportSettings.getTypeName());

		BeanHusk huskDiscovery = getProp(huskNportSettings, NPortSettings::getDiscovery);
		assertEquals(Boolean.class.getSimpleName(), huskDiscovery.getTypeName());
		assertEquals(null, huskDiscovery.getValue());

		BeanHusk huskAdaptersByIpStr = getProp(rootHusk, GtwayV24Data::getAdaptersByIpStr);
		assertEquals("Map<String, EtherSerialAdapter>", huskAdaptersByIpStr.getTypeName());

		BeanHusk huskEntryAdapter = huskAdaptersByIpStr.getProperties().get("0");
		assertEquals("Map$Entry<String, EtherSerialAdapter>", huskEntryAdapter.getTypeName());
		assertEquals(IP_ADDR, huskEntryAdapter.getDisplayName());

		BeanHusk huskAdapter = getProp(huskEntryAdapter, Map.Entry::getValue);
		assertEquals(EtherSerialAdapter.class.getSimpleName(), huskAdapter.getTypeName());

		assertEquals("test adapter", getProp(huskAdapter, EtherSerialAdapter::getName).getValue());

		BeanHusk huskSerialsByTcpServerPort = getProp(huskAdapter, EtherSerialAdapter::getSerialsByTcpServerPort);
		assertEquals("Map<Integer, EtherSerialPort>", huskSerialsByTcpServerPort.getTypeName());

		BeanHusk huskEntryPortObj = huskSerialsByTcpServerPort.getProperties().get("0");
		assertEquals("Map$Entry<Integer, EtherSerialPort>", huskEntryPortObj.getTypeName());
		assertEquals(TCP_PORT.toString(), huskEntryPortObj.getDisplayName());

		BeanHusk huskPortObj = getProp(huskEntryPortObj, Map.Entry::getValue);
		assertEquals(EtherSerialPort.class.getSimpleName(), huskPortObj.getTypeName());
		assertEquals(966, getProp(huskPortObj, EtherSerialPort::getNportCommandPort).getValue());

		BeanHusk huskBus = getProp(huskPortObj, EtherSerialPort::getBus);
		SerialBus<?> bus = rootObj.getAdaptersByIpStr().get(IP_ADDR)
				.getSerialsByTcpServerPort().get(TCP_PORT)
				.getBus();
		assertEquals("SerialBus<?>", huskBus.getTypeName());
		assertEquals(9600, getProp(huskBus, SerialBus::getBaudRate).getValue());
		assertEquals('2', getProp(huskBus, EspaBus::getOurAddress).getValue());

		BeanHusk huskEndpointsByBusAddr = getProp(huskBus, EspaBus::getEndpointsByBusAddr);
		
		Map<String, ? extends V24Device> endpoints = bus.getEndpointsByBusAddr();
		assertEquals("Map<String, ? extends V24Device>", huskEndpointsByBusAddr.getTypeName());

		BeanHusk huskEntryEndpoint = huskEndpointsByBusAddr.getProperties().get("0");
		assertEquals("Map$Entry<String, ? extends V24Device>", huskEntryEndpoint.getTypeName());
		assertEquals(BUS_ADDR, huskEntryEndpoint.getDisplayName());

		BeanHusk huskEndpoint = getProp(huskEntryEndpoint, Map.Entry::getValue);
		V24Device dev = endpoints.get(BUS_ADDR);
		dev.toString();
		assertEquals("? extends V24Device", huskEndpoint.getTypeName());
		assertEquals(1001, getProp(huskEndpoint, V24Device::getV24DeviceId).getValue());
	}

	private GtwayV24Data rootObj;

	@SuppressWarnings("deprecation")
	@Override
	protected void setUp() throws Exception {
		super.setUp();
		rootObj = new GtwayV24Data();
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
	}

	private interface GetterGtwayV24Data extends Function<GtwayV24Data, Object> {}
	private interface GetterNPortSettings extends Function<NPortSettings, Object> {}
	private interface GetterEtherSerialAdapter extends Function<EtherSerialAdapter, Object> {}
	private interface GetterEtherSerialPort extends Function<EtherSerialPort, Object> {}
	private interface GetterEspaBus extends Function<EspaBus, Object> {}
	private interface GetterV24Device extends Function<V24Device, Object> {}
	private interface GetterMapEntry extends Function<Map.Entry, Object> {}

	private static BeanHusk getProp(BeanHusk husk, GetterMapEntry getter) {
		return getProp0(husk, getter);
	}

	private static BeanHusk getProp(BeanHusk husk, GetterV24Device getter) {
		return getProp0(husk, getter);
	}

	private static BeanHusk getProp(BeanHusk husk, GetterEspaBus getter) {
		return getProp0(husk, getter);
	}

	private static BeanHusk getProp(BeanHusk husk, GetterEtherSerialPort getter) {
		return getProp0(husk, getter);
	}
	
	private static BeanHusk getProp(BeanHusk husk, GetterEtherSerialAdapter getter) {
		return getProp0(husk, getter);
	}

	private static BeanHusk getProp(BeanHusk husk, GetterNPortSettings getter) {
		return getProp0(husk, getter);
	}

	private static BeanHusk getProp(BeanHusk husk, GetterGtwayV24Data getter) {
		return getProp0(husk, getter);
	}

	private static <T> BeanHusk getProp0(BeanHusk husk, final Function<T, ?> getter) {
		return husk.getProperties().get(getPropName0(getter));
	}

	private static <T> String getPropName0(final Function<T, ?> getter) {
		String mname = getMethodName0(getter);
		String base;
		if (mname.startsWith("get")) {
			base = mname.substring(3);
		} else if (mname.startsWith("is")) {
			base = mname.substring(2);
		} else {
			throw new IllegalArgumentException("Not a getter: " + mname);
		}
		return Introspector.decapitalize(base);
	}

	private static <T> String getMethodName0(final Function<T, ?> getter) {
		@SuppressWarnings("unchecked")
		Class<Function<T, ?>> getterClazz = (Class<Function<T, ?>>)getter.getClass();
		TypeToken<Function<T, ?>> tt = TypeToken.of(getterClazz);
		TypeToken<? super Function<T, ?>> ft = tt.getSupertype(Function.class);
		@SuppressWarnings("unchecked")
		Class<T> beanClazz = (Class<T>)((ParameterizedType)ft.getType()).getActualTypeArguments()[0];
		return getMethodName(beanClazz, getter);
	}

	private static <T> String getMethodName(final Class<T> clazz, final Function<T, ?> getter) {
		final Method[] method = new Method[1];
		T mock = Mockito.mock(clazz, Mockito.withSettings().invocationListeners(methodInvocationReport -> {
			method[0] = ((InvocationOnMock) methodInvocationReport.getInvocation()).getMethod();
		}));
		getter.apply(mock);
		return method[0].getName();
	}
}
