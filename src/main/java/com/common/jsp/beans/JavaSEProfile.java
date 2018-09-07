package com.common.jsp.beans;

import java.util.Arrays;
import java.util.Comparator;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.SortedSet;
import java.util.TreeSet;

public class JavaSEProfile {

	public static final Comparator<Class<?>> WHITEBLACK_COMPARATOR = new Comparator<Class<?>>() {

		@Override
		public int compare(Class<?> o1, Class<?> o2) {
			if (o1 == o2) {
				return 0;
			}
			if (o1 == null) {
				return -1;
			}
			if (o2 == null) {
				return 1;
			}
			
			Class<?> thisComponentType = o1.getComponentType();
			Class<?> otherComponentType = o2.getComponentType();
			
			int res = compare(thisComponentType, otherComponentType);
			if (res != 0 || thisComponentType != null) {
				return res;
			}
			
			String firstPackage = getPackage(o1.getName());
			String secondPackage = getPackage(o2.getName());
			Integer thisWhite = JavaSEProfile.WHITELIST_PACKAGES.get(firstPackage);
			Integer otherWhite = JavaSEProfile.WHITELIST_PACKAGES.get(secondPackage);

			if (thisWhite == null && otherWhite != null) {
				return 1;
			}

			if (thisWhite != null && otherWhite == null) {
				return -1;
			}
			
			if (thisWhite != null && otherWhite != null) {
				if (thisWhite.intValue() > otherWhite.intValue()) {
					return 1;
				}
				if (thisWhite.intValue() < otherWhite.intValue()) {
					return -1;
				}
				return ClassComparator.INSTANCE.compare(o1, o2);
			}
			
			boolean firstBlacklisted = isBlackListed(o1.getName());
			boolean secondBlacklisted = isBlackListed(o2.getName());
			
			if (firstBlacklisted && !secondBlacklisted) {
				return 1;
			}
			
			if (!firstBlacklisted && secondBlacklisted) {
				return -1;
			}
			
			return ClassComparator.INSTANCE.compare(o1, o2);
		}
	};

	private static boolean isBlackListed(String className) {
		for (String x : BLACKLIST_PREFIXES) {
			if (className.startsWith(x)) {
				return true;
			}
		}
		return false;
	}
	
	public static String getPackage(String name) {
		int i = name.lastIndexOf('.');
		if (i != -1) {
			name = name.substring(0, i);
			return name;
		} else {
			return null;
		}
	}

	public static final Map<String, Integer> WHITELIST_PACKAGES;
	
	private static final String[] _WHITELIST_PACKAGES = {
		"java.lang",
		"java.util",

		"java.applet",
		"java.awt",
		"java.awt.color",
		"java.awt.datatransfer",
		"java.awt.dnd",
		"java.awt.event",
		"java.awt.font",
		"java.awt.geom",
		"java.awt.im",
		"java.awt.im.spi",
		"java.awt.image",
		"java.awt.image.renderable",
		"java.awt.print",
		"java.beans",
		"java.beans.beancontext",
		"java.io",
		"java.lang.annotation",
		"java.lang.instrument",
		"java.lang.invoke",
		"java.lang.management",
		"java.lang.ref",
		"java.lang.reflect",
		"java.math",
		"java.net",
		"java.nio",
		"java.nio.channels",
		"java.nio.channels.spi",
		"java.nio.charset",
		"java.nio.charset.spi",
		"java.nio.file",
		"java.nio.file.attribute",
		"java.nio.file.spi",
		"java.rmi",
		"java.rmi.activation",
		"java.rmi.dgc",
		"java.rmi.registry",
		"java.rmi.server",
		"java.security",
		"java.security.acl",
		"java.security.cert",
		"java.security.interfaces",
		"java.security.spec",
		"java.sql",
		"java.text",
		"java.text.spi",
		"java.time",
		"java.time.chrono",
		"java.time.format",
		"java.time.temporal",
		"java.time.zone",
		"java.util.concurrent",
		"java.util.concurrent.atomic",
		"java.util.concurrent.locks",
		"java.util.function",
		"java.util.jar",
		"java.util.logging",
		"java.util.prefs",
		"java.util.regex",
		"java.util.spi",
		"java.util.stream",
		"java.util.zip",
		"javax.accessibility",
		"javax.activation",
		"javax.activity",
		"javax.annotation",
		"javax.annotation.processing",
		"javax.crypto",
		"javax.crypto.interfaces",
		"javax.crypto.spec",
		"javax.imageio",
		"javax.imageio.event",
		"javax.imageio.metadata",
		"javax.imageio.plugins.bmp",
		"javax.imageio.plugins.jpeg",
		"javax.imageio.spi",
		"javax.imageio.stream",
		"javax.jws",
		"javax.jws.soap",
		"javax.lang.model",
		"javax.lang.model.element",
		"javax.lang.model.type",
		"javax.lang.model.util",
		"javax.management",
		"javax.management.loading",
		"javax.management.modelmbean",
		"javax.management.monitor",
		"javax.management.openmbean",
		"javax.management.relation",
		"javax.management.remote",
		"javax.management.remote.rmi",
		"javax.management.timer",
		"javax.naming",
		"javax.naming.directory",
		"javax.naming.event",
		"javax.naming.ldap",
		"javax.naming.spi",
		"javax.net",
		"javax.net.ssl",
		"javax.print",
		"javax.print.attribute",
		"javax.print.attribute.standard",
		"javax.print.event",
		"javax.rmi",
		"javax.rmi.CORBA",
		"javax.rmi.ssl",
		"javax.script",
		"javax.security.auth",
		"javax.security.auth.callback",
		"javax.security.auth.kerberos",
		"javax.security.auth.login",
		"javax.security.auth.spi",
		"javax.security.auth.x500",
		"javax.security.cert",
		"javax.security.sasl",
		"javax.sound.midi",
		"javax.sound.midi.spi",
		"javax.sound.sampled",
		"javax.sound.sampled.spi",
		"javax.sql",
		"javax.sql.rowset",
		"javax.sql.rowset.serial",
		"javax.sql.rowset.spi",
		"javax.swing",
		"javax.swing.border",
		"javax.swing.colorchooser",
		"javax.swing.event",
		"javax.swing.filechooser",
		"javax.swing.plaf",
		"javax.swing.plaf.basic",
		"javax.swing.plaf.metal",
		"javax.swing.plaf.multi",
		"javax.swing.plaf.nimbus",
		"javax.swing.plaf.synth",
		"javax.swing.table",
		"javax.swing.text",
		"javax.swing.text.html",
		"javax.swing.text.html.parser",
		"javax.swing.text.rtf",
		"javax.swing.tree",
		"javax.swing.undo",
		"javax.tools",
		"javax.transaction",
		"javax.transaction.xa",
		"javax.xml",
		"javax.xml.bind",
		"javax.xml.bind.annotation",
		"javax.xml.bind.annotation.adapters",
		"javax.xml.bind.attachment",
		"javax.xml.bind.helpers",
		"javax.xml.bind.util",
		"javax.xml.crypto",
		"javax.xml.crypto.dom",
		"javax.xml.crypto.dsig",
		"javax.xml.crypto.dsig.dom",
		"javax.xml.crypto.dsig.keyinfo",
		"javax.xml.crypto.dsig.spec",
		"javax.xml.datatype",
		"javax.xml.namespace",
		"javax.xml.parsers",
		"javax.xml.soap",
		"javax.xml.stream",
		"javax.xml.stream.events",
		"javax.xml.stream.util",
		"javax.xml.transform",
		"javax.xml.transform.dom",
		"javax.xml.transform.sax",
		"javax.xml.transform.stax",
		"javax.xml.transform.stream",
		"javax.xml.validation",
		"javax.xml.ws",
		"javax.xml.ws.handler",
		"javax.xml.ws.handler.soap",
		"javax.xml.ws.http",
		"javax.xml.ws.soap",
		"javax.xml.ws.spi",
		"javax.xml.ws.spi.http",
		"javax.xml.ws.wsaddressing",
		"javax.xml.xpath",
		"org.ietf.jgss",
		"org.omg.CORBA",
		"org.omg.CORBA_2_3",
		"org.omg.CORBA_2_3.portable",
		"org.omg.CORBA.DynAnyPackage",
		"org.omg.CORBA.ORBPackage",
		"org.omg.CORBA.portable",
		"org.omg.CORBA.TypeCodePackage",
		"org.omg.CosNaming",
		"org.omg.CosNaming.NamingContextExtPackage",
		"org.omg.CosNaming.NamingContextPackage",
		"org.omg.Dynamic",
		"org.omg.DynamicAny",
		"org.omg.DynamicAny.DynAnyFactoryPackage",
		"org.omg.DynamicAny.DynAnyPackage",
		"org.omg.IOP",
		"org.omg.IOP.CodecFactoryPackage",
		"org.omg.IOP.CodecPackage",
		"org.omg.Messaging",
		"org.omg.PortableInterceptor",
		"org.omg.PortableInterceptor.ORBInitInfoPackage",
		"org.omg.PortableServer",
		"org.omg.PortableServer.CurrentPackage",
		"org.omg.PortableServer.POAManagerPackage",
		"org.omg.PortableServer.POAPackage",
		"org.omg.PortableServer.portable",
		"org.omg.PortableServer.ServantLocatorPackage",
		"org.omg.SendingContext",
		"org.omg.stub.java.rmi",
		"org.w3c.dom",
		"org.w3c.dom.bootstrap",
		"org.w3c.dom.css",
		"org.w3c.dom.events",
		"org.w3c.dom.html",
		"org.w3c.dom.ls",
		"org.w3c.dom.ranges",
		"org.w3c.dom.stylesheets",
		"org.w3c.dom.traversal",
		"org.w3c.dom.views",
		"org.w3c.dom.xpath",
		"org.xml.sax",
		"org.xml.sax.ext",
		"org.xml.sax.helpers"
	};

	static {
		WHITELIST_PACKAGES = new HashMap<>(_WHITELIST_PACKAGES.length);
		for (int i = 0; i < _WHITELIST_PACKAGES.length; i++) {
			WHITELIST_PACKAGES.put(_WHITELIST_PACKAGES[i], i);
		}
	}

	public static final String[] BLACKLIST_PREFIXES = {
		"com.oracle.",
		"com.sun.",
		"java.",
		"javax.",
		"jdk.",
		"org.jcp.xml.dsig.internal.",
		"org.omg.stub.javax.",
		"sun."
	};
}
