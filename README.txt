java.sql classes from openjdk7 having '@since 1.6' or '@since 1.7' in their javadoc and recompiled with <target>1.5</target>.
Add the resulting jar to javac -bootclasspath.
An attempt to use new methods on old JRE will result AbstractMethodError or NoSuchMethodError.
