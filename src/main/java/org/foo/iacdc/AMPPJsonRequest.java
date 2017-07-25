package org.foo.iacdc;

import java.util.Date;

public class AMPPJsonRequest {
    
    private String request_id, application_id;
    
    private Date timestamp;
    
    private String journal_id, year, article_id, in_package_name, in_package_location;
    
    private PackageState package_state;
    
    private String user_name, response_routing_key, out_package_location;
    
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
    
    public Date getTimestamp() {
        return timestamp;
    }
    
    public void setTimestamp(final Date timestamp) {
        this.timestamp = timestamp;
    }
    
    public String getJournal_id() {
        return journal_id;
    }
    
    public void setJournal_id(final String journal_id) {
        this.journal_id = journal_id;
    }
    
    public String getYear() {
        return year;
    }
    
    public void setYear(final String year) {
        this.year = year;
    }
    
    public String getArticle_id() {
        return article_id;
    }
    
    public void setArticle_id(final String article_id) {
        this.article_id = article_id;
    }
    
    public String getIn_package_name() {
        return in_package_name;
    }
    
    public void setIn_package_name(final String package_name) {
        this.in_package_name = package_name;
    }
    
    public String getIn_package_location() {
        return in_package_location;
    }
    
    public void setIn_package_location(final String package_location) {
        this.in_package_location = package_location;
    }
    
    public PackageState getPackage_state() {
        return package_state;
    }
    
    public void setPackage_state(final PackageState package_state) {
        this.package_state = package_state;
    }
    
    public String getUser_name() {
        return user_name;
    }
    
    public void setUser_name(final String user_name) {
        this.user_name = user_name;
    }
    
    public String getResponse_routing_key() {
        return response_routing_key;
    }
    
    public void setResponse_routing_key(final String response_routing_key) {
        this.response_routing_key = response_routing_key;
    }
    
    public String getOut_package_location() {
        return out_package_location;
    }
    
    public void setOut_package_location(final String out_package_location) {
        this.out_package_location = out_package_location;
    }
    
}
