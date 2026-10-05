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
				public class Fields {
					public static final int VALID = 1;
					public static final int INVALID$FIELD = 2;
					public static final int from = 3;
				}
				""");
			DocumentationTool tool = ToolProvider.getSystemDocumentationTool();
			try (StandardJavaFileManager files = tool.getStandardFileManager(null, null, null)) {
				StringWriter output = new StringWriter();
				assertTrue(output.toString(), tool.getTask(output, files, null,
					PythonTypeStubDoclet.class, List.of("-d", root.resolve("stubs").toString()),
					files.getJavaFileObjects(source)).call());
			}
			String stub = Files.readString(root.resolve("stubs/fixture-stubs/__init__.pyi"));
			assertTrue(stub.contains("VALID: typing.Final = 1"));
			assertTrue(stub.contains("from_: typing.Final = 3"));
			assertFalse(stub.contains("INVALID$FIELD"));
			assertFalse(stub.contains("INVALID_FIELD"));
		}
		finally {
			try (var paths = Files.walk(root)) {
				for (Path path : paths.sorted(java.util.Comparator.reverseOrder()).toList()) {
					Files.delete(path);
				}
			}
		}
	}
}
