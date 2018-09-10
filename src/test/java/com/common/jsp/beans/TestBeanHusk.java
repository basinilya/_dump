package com.common.jsp.beans;

import java.beans.BeanInfo;
import java.beans.Introspector;
import java.beans.PropertyDescriptor;
import java.lang.reflect.Method;
import java.lang.reflect.ParameterizedType;
import java.lang.reflect.Type;
import java.lang.reflect.TypeVariable;
import java.util.AbstractMap;
import java.util.Arrays;
import java.util.List;
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
import com.common.test.v24.V24ProtocolEm;
import com.google.common.reflect.TypeToken;

import org.junit.Assert;

import junit.framework.TestCase;
import static com.common.jsp.beans.TestService.*;

public class TestBeanHusk extends TestCase {

	static boolean skipThis = false;

	@SuppressWarnings("deprecation")
	public void testBeanHusk() throws Exception {
		rootObj.getAdaptersByIpStr();
		rootObj.getNportSettings();
		rootObj.getV24EndpointSeq();
		
		BeanHusk rootHusk = new BeanHusk(rootObj);
		

		assertEquals(GtwayV24Data.class.getSimpleName(), rootHusk.getTypeName());
		assertTrue(rootHusk.getValueAsText().matches(".*[.]"+GtwayV24Data.class.getSimpleName()+".*"));
		
		BeanHusk huskV24EndpointSeq = getProp(rootHusk, GtwayV24Data::getV24EndpointSeq);


		BeanHusk huskNportSettings = getProp(rootHusk, GtwayV24Data::getNportSettings);
		assertEquals(NPortSettings.class.getSimpleName(), huskNportSettings.getTypeName());
		assertTrue(huskNportSettings.getValueAsText().matches(".*[.]"+NPortSettings.class.getSimpleName()+".*"));


		BeanHusk huskDiscovery = getProp(huskNportSettings, NPortSettings::getDiscovery);
		assertEquals(Boolean.class.getSimpleName(), huskDiscovery.getTypeName());
		assertEquals(null, huskDiscovery.getValue());
		assertEquals(null, huskDiscovery.getValueAsText());

		BeanHusk huskAdaptersByIpStr = getProp(rootHusk, GtwayV24Data::getAdaptersByIpStr);
		assertEquals("Map<String, EtherSerialAdapter>", huskAdaptersByIpStr.getTypeName());
		assertTrue(huskAdaptersByIpStr.getValueAsText().matches("[{].*[=].*[}]"));

		BeanHusk huskEntryAdapter = huskAdaptersByIpStr.getProperties().get("0");
		assertEquals("Map$Entry<String, EtherSerialAdapter>", huskEntryAdapter.getTypeName());
		assertEquals(IP_ADDR, huskEntryAdapter.getDisplayName());
		assertTrue(huskEntryAdapter.getValueAsText().matches(".*[=].*"));

		BeanHusk huskAdapter = getProp(huskEntryAdapter, Map.Entry::getValue);
		assertTrue(huskAdapter.isSetValueSupported());
		assertEquals(EtherSerialAdapter.class.getSimpleName(), huskAdapter.getTypeName());
		assertTrue(huskAdapter.getValueAsText().matches(".*[.]"+EtherSerialAdapter.class.getSimpleName()+".*"));

		assertEquals("test adapter", getProp(huskAdapter, EtherSerialAdapter::getName).getValue());

		BeanHusk huskSerialsByTcpServerPort = getProp(huskAdapter, EtherSerialAdapter::getSerialsByTcpServerPort);
		assertEquals("Map<Integer, EtherSerialPort>", huskSerialsByTcpServerPort.getTypeName());
		assertTrue(huskSerialsByTcpServerPort.getValueAsText().matches("[{].*[=].*[}]"));

		BeanHusk huskEntryPortObj = huskSerialsByTcpServerPort.getProperties().get("0");
		assertEquals("Map$Entry<Integer, EtherSerialPort>", huskEntryPortObj.getTypeName());
		assertEquals(TCP_PORT.toString(), huskEntryPortObj.getDisplayName());
		assertTrue(huskEntryPortObj.getValueAsText().matches(".*[=].*"));

		BeanHusk huskPortObj = getProp(huskEntryPortObj, Map.Entry::getValue);
		assertEquals(EtherSerialPort.class.getSimpleName(), huskPortObj.getTypeName());
		assertTrue(huskPortObj.getValueAsText().matches(".*[.]"+EtherSerialPort.class.getSimpleName()+".*"));
		assertEquals(966, getProp(huskPortObj, EtherSerialPort::getNportCommandPort).getValue());

		BeanHusk huskBus = getProp(huskPortObj, EtherSerialPort::getBus);
		SerialBus<?> bus = rootObj.getAdaptersByIpStr().get(IP_ADDR)
				.getSerialsByTcpServerPort().get(TCP_PORT)
				.getBus();
		assertEquals("SerialBus<?>", huskBus.getTypeName());
		assertTrue(huskBus.getValueAsText().matches(".*[.]"+EspaBus.class.getSimpleName()+".*"));
		assertEquals(9600, getProp(huskBus, SerialBus::getBaudRate).getValue());
		assertEquals('2', getProp(huskBus, EspaBus::getOurAddress).getValue());
		assertEquals(V24ProtocolEm.ESPA, getProp(huskBus, SerialBus::getV24Protocol).getValue());

		BeanHusk huskEndpointsByBusAddr = getProp(huskBus, EspaBus::getEndpointsByBusAddr);
		
		Map<String, ? extends V24Device> endpoints = bus.getEndpointsByBusAddr();
		assertEquals("Map<String, ? extends V24Device>", huskEndpointsByBusAddr.getTypeName());
		assertTrue(huskEndpointsByBusAddr.getValueAsText().matches("[{].*[=].*[}]"));

		BeanHusk huskEntryEndpoint = huskEndpointsByBusAddr.getProperties().get("0");
		assertEquals("Map$Entry<String, ? extends V24Device>", huskEntryEndpoint.getTypeName());
		assertTrue(huskEntryEndpoint.getValueAsText().matches(".*[=].*"));
		assertEquals(BUS_ADDR, huskEntryEndpoint.getDisplayName());

		BeanHusk huskEndpoint = getProp(huskEntryEndpoint, Map.Entry::getValue);
		V24Device dev = endpoints.get(BUS_ADDR);
		dev.toString();
		assertEquals("? extends V24Device", huskEndpoint.getTypeName());
		assertTrue(huskEndpoint.getValueAsText().matches(".*[.]"+V24Device.class.getSimpleName()+".*"));
		assertEquals(1001, getProp(huskEndpoint, V24Device::getV24DeviceId).getValue());

		if (!skipThis) {
			assertTrue(huskDiscovery.isSetValueSupported());
			Map<String, List<Factory>> factories = huskDiscovery.getFactories();
			assertEquals(0, factories.get(String.class.getName()).size());
			List<Factory> factoriesRestricted = factories.get(Boolean.class.getName());
			assertTrue(factoriesRestricted.size() > 1);

			List<Factory> factoriesAll = factories.get(null);
			assertTrue(factoriesAll.size() > 1);
			
			Factory factory = factoriesAll.get(0);
			Object option = factory.getTags()[0];
			Object newVal = factoriesAll.get(0).getInstance(new Object[] { option } );
			huskDiscovery.setValue(newVal);
			assertSame(newVal, rootObj.getNportSettings().getDiscovery());

			String xxx = "new java.lang.Boolean(boolean)";
			for (int i = 1;; i++) {
				if (i >= factoriesAll.size()) {
					fail();
				}
				factory = factoriesAll.get(i);
				String name = factory.toString();
				if (xxx.equals(name)) {
					FactoryProvider arg0 = factory.getParamsProviders().get(0);
					Object arg0val = arg0.getFactories().get(null).get(0).getInstance(new Object[] { Boolean.TRUE.toString() });
					factory.getInstance(new Object[] { arg0val });
					break;
				}
			}
		}

		if (!skipThis) {
			assertTrue(huskNportSettings.isSetValueSupported());
			Map<String, List<Factory>> factories = huskNportSettings.getFactories();
			assertEquals(0, factories.get(String.class.getName()).size());
			List<Factory> factoriesRestricted = factories.get(NPortSettings.class.getName());
			assertEquals(1, factoriesRestricted.size());

			List<Factory> factoriesAll = factories.get(null);
			assertEquals(1, factoriesAll.size());
			Object newVal = factoriesAll.get(0).getInstance(new Object[] { } );
			huskNportSettings.setValue(newVal);

			assertSame(newVal, rootObj.getNportSettings());
		}

		if (!skipThis) {
			assertTrue(huskV24EndpointSeq.isSetValueSupported());
			Map<String, List<Factory>> v24EndpointSeqFactories = huskV24EndpointSeq.getFactories();
			assertEquals(0, v24EndpointSeqFactories.get(String.class.getName()).size());
			List<Factory> factoriesRestricted = v24EndpointSeqFactories.get(int.class.getName());
			assertEquals(0, factoriesRestricted.size());

			List<Factory> factoriesAll = v24EndpointSeqFactories.get(null);
			assertEquals(1, factoriesAll.size());
			int i = 42;
			Object newVal = factoriesAll.get(0).getInstance(new Object[] { Integer.toString(i) } );
			try {
				huskV24EndpointSeq.setValue(null);
				fail();
			} catch (Exception e) {
				// expected
			}
			huskV24EndpointSeq.setValue(newVal);

			assertEquals(newVal, rootObj.getV24EndpointSeq());
		}
		
		if (!skipThis) {
			assertTrue(rootHusk.isSetValueSupported());
			Map<String, List<Factory>> rootFactories = rootHusk.getFactories();
			assertEquals(0, rootFactories.get(String.class.getName()).size());
			List<Factory> rootFactoriesAll = rootFactories.get(null);
			assertEquals(1, rootFactoriesAll.size());
			List<Factory> rootFactoriesRestricted = rootFactories.get(GtwayV24Data.class.getName());
			assertEquals(1, rootFactoriesRestricted.size());
			rootFactoriesRestricted.get(0).getInstance(new Object[0]);
		}rootHusk.getTypeToken().isPrimitive();

	}

	private GtwayV24Data rootObj;

	@Override
	protected void setUp() throws Exception {
		super.setUp();
		rootObj = TestService.getGtwayV24Data();
		FactoryProvider.testFilter = s -> false;
	}

	@Override
	protected void tearDown() throws Exception {
		FactoryProvider.testFilter = null;
		super.tearDown();
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
		String propName = getPropName0(getter);
		BeanHusk res = husk.getProperties().get(propName);
		Assert.assertEquals(propName, res.getDisplayName());
		return res;
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
