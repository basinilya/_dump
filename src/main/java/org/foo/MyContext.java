package org.foo;

import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.Map;

public class MyContext {
	private final Map<String, RetrieveWorker> workersByFilename = Collections.synchronizedMap(new LinkedHashMap<String, RetrieveWorker>());

	public Map<String, RetrieveWorker> getWorkersByFilename() {
		return workersByFilename;
	}
}
