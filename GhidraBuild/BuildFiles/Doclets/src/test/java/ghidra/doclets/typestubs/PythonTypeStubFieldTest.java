/* ###
 * IP: GHIDRA
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package ghidra.doclets.typestubs;

import static org.junit.Assert.*;

import java.io.StringWriter;
import java.nio.file.*;
import java.util.List;
import javax.tools.*;

import org.junit.Test;

public class PythonTypeStubFieldTest {
	@Test
	public void excludesFieldsOnlyAccessibleThroughGetattr() throws Exception {
		Path root = Files.createTempDirectory("stub-field-test");
		try {
			Path source = root.resolve("Fields.java");
			Files.writeString(source, """
				package fixture;
				public class Fields extends Fields$Shared {
					public static final int VALID = 1;
					public static final int INVALID$FIELD = 2;
					public static final int from = 3;
					public static int layout$() { return 4; }
					public static Fields$Shared shared() { return null; }
				}
				class Fields$Shared {}
				""");
			render(source, root.resolve("stubs"));
			String stub = Files.readString(root.resolve("stubs/fixture-stubs/__init__.pyi"));
			assertTrue(stub.contains("VALID: typing.Final = 1"));
			assertTrue(stub.contains("from_: typing.Final = 3"));
			assertFalse(stub.contains("INVALID$FIELD"));
			assertFalse(stub.contains("INVALID_FIELD"));
			assertFalse(stub.contains("def layout$"));
			assertFalse(stub.contains("class Fields$Shared"));
			assertTrue(stub.contains("class Fields(typing.Any)"));
			assertTrue(stub.contains("-> typing.Any:"));
		}
		finally {
			try (var paths = Files.walk(root)) {
				for (Path path : paths.sorted(java.util.Comparator.reverseOrder()).toList()) {
					Files.delete(path);
				}
			}
		}
	}
	@Test
	public void removesRedundantInterfaceAncestors() throws Exception {
		Path root = Files.createTempDirectory("stub-interface-test");
		try {
			Path source = root.resolve("Interfaces.java");
			Files.writeString(source, """
				package fixture;
				public class Interfaces {
					public interface Root { void release(); }
					public interface Store extends Root {}
					public interface Other {}
					public interface View extends Root, Other, Store {}
					public static class Parent implements Root { public void release() {} }
					public static class Child extends Parent implements Root {}
				}
				""");
			String stub = render(source, root.resolve("stubs"));
			assertTrue(stub.contains("class View(Interfaces.Other, Interfaces.Store)"));
			assertTrue(stub.contains("class Child(Interfaces.Parent)"));
			assertTrue(stub.contains("class Store(Interfaces.Root)"));
		}
		finally {
			try (var paths = Files.walk(root)) {
				for (Path path : paths.sorted(java.util.Comparator.reverseOrder()).toList()) {
					Files.delete(path);
				}
			}
		}
	}

	private static String render(Path source, Path dest) throws Exception {
		DocumentationTool tool = ToolProvider.getSystemDocumentationTool();
		try (StandardJavaFileManager files = tool.getStandardFileManager(null, null, null)) {
			StringWriter output = new StringWriter();
			boolean success = tool.getTask(output, files, null, PythonTypeStubDoclet.class,
				List.of("-d", dest.toString()), files.getJavaFileObjects(source)).call();
			assertTrue(output.toString(), success);
		}
		return Files.readString(dest.resolve("fixture-stubs/__init__.pyi"));
	}

}
