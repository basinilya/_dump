package org.foo.iacdc;

import java.io.InputStream;
import java.net.URL;
import java.util.Date;
import java.util.Set;

import junit.framework.TestCase;

import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.networknt.schema.JsonSchema;
import com.networknt.schema.JsonSchemaFactory;
import com.networknt.schema.ValidationMessage;

public class IACDCTest extends TestCase {
    
    /*
     * Durable reply queue: We need a persistent name for it, unique across all machines, but all
     * Dev VMs are clones, so we cannot guarantee uniqueness. If we simply reject a foreign reply
     * with requeue=true, it will be inefficient, besides there must be
     */
    
    public static void main1(final String[] args) throws Exception {
        new IACDCTest().testDoIt();
    }
    
    public void testDoIt() throws Exception {
        final ObjectMapper mapper = new ObjectMapper();
        
        AMPPJsonRequest req = null;
        
        req =
                mapper.readValue(IACDCTest.class.getResourceAsStream("examples/ampp1.json"),
                        AMPPJsonRequest.class);
        
        if ("".length() == 1) {
            req = new AMPPJsonRequest();
            req.setApplication_id("x");
            req.setArticle_id("x");
            req.setJournal_id("x");
            req.setPackage_location("x");
            req.setPackage_name("x");
            req.setPackage_state(PackageState.language_editing);
            req.setRequest_id("x");
            req.setTimestamp(new Date());
            req.setUser_name("x");
            req.setYear("x");
        }
        
        mapper.writerWithDefaultPrettyPrinter().writeValue(System.out, req);
        
        testDoIt("schema/ampp-request.schema.json", "examples/ampp-request_simple.json");
        testDoIt("schema/ampp.schema.json", "examples/ampp_simple.json");
        testDoIt("schema/reject.schema.json", "examples/reject_no_pointer.json");
        testDoIt("schema/reject.schema.json", "examples/reject_pointers.json");
    }
    
    private void testDoIt(final String schemaName, final String jsonName) throws Exception {
        final JsonSchema schema = getJsonSchemaFromClasspath(schemaName);
        final JsonNode node = getJsonNodeFromClasspath(jsonName);
        final Set<ValidationMessage> errors = schema.validate(node);
        for (final ValidationMessage msg : errors) {
            System.err.println(msg);
        }
        assertEquals(0, errors.size());
    }
    
    private JsonNode getJsonNodeFromClasspath(final String name) throws Exception {
        final InputStream is1 = IACDCTest.class.getResourceAsStream(name);
        
        final ObjectMapper mapper = new ObjectMapper();
        final JsonNode node = mapper.readTree(is1);
        return node;
    }
    
    private JsonNode getJsonNodeFromStringContent(final String content) throws Exception {
        final ObjectMapper mapper = new ObjectMapper();
        final JsonNode node = mapper.readTree(content);
        return node;
    }
    
    private JsonNode getJsonNodeFromUrl(final String url) throws Exception {
        final ObjectMapper mapper = new ObjectMapper();
        final JsonNode node = mapper.readTree(new URL(url));
        return node;
    }
    
    private JsonSchema getJsonSchemaFromClasspath(final String name) throws Exception {
        final JsonSchemaFactory factory = new JsonSchemaFactory();
        final InputStream is = IACDCTest.class.getResourceAsStream(name);
        // final InputStream is =
        // Thread.currentThread().getContextClassLoader().getResourceAsStream(name);
        final JsonSchema schema = factory.getSchema(is);
        return schema;
    }
    
    private JsonSchema getJsonSchemaFromStringContent(final String schemaContent) throws Exception {
        final JsonSchemaFactory factory = new JsonSchemaFactory();
        final JsonSchema schema = factory.getSchema(schemaContent);
        return schema;
    }
    
    private JsonSchema getJsonSchemaFromUrl(final String url) throws Exception {
        final JsonSchemaFactory factory = new JsonSchemaFactory();
        final JsonSchema schema = factory.getSchema(new URL(url));
        return schema;
    }
    
    private JsonSchema getJsonSchemaFromJsonNode(final JsonNode jsonNode) throws Exception {
        final JsonSchemaFactory factory = new JsonSchemaFactory();
        final JsonSchema schema = factory.getSchema(jsonNode);
        return schema;
    }
}
