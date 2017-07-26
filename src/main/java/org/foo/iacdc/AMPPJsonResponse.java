package org.foo.iacdc;

public class AMPPJsonResponse {
    
    private Integer try_no;
    
    private String request_id, application_id, timestamp, package_name, package_location, state,
            message;
    
    public String getRequest_id() {
        return request_id;
    }
    
    public void setRequest_id(final String request_id) {
        this.request_id = request_id;
    }
    
    public String getApplication_id() {
        return application_id;
    }
    
    public void setApplication_id(final String application_id) {
        this.application_id = application_id;
    }
    
    public String getTimestamp() {
        return timestamp;
    }
    
    public void setTimestamp(final String timestamp) {
        this.timestamp = timestamp;
    }
    
    public String getPackage_name() {
        return package_name;
    }
    
    public void setPackage_name(final String package_name) {
        this.package_name = package_name;
    }
    
    public String getPackage_location() {
        return package_location;
    }
    
    public void setPackage_location(final String package_location) {
        this.package_location = package_location;
    }
    
    public String getState() {
        return state;
    }
    
    public void setState(final String state) {
        this.state = state;
    }
    
    public String getMessage() {
        return message;
    }
    
    public void setMessage(final String message) {
        this.message = message;
    }
    
    public Integer getTry_no() {
        return try_no;
    }
    
    public void setTry_no(final Integer try_no) {
        this.try_no = try_no;
    }
}
