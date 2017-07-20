package org.foo.iacdc;

public class AMPPJsonRequest {
    
    private String request_id, application_id, timestamp, journal_id, year, article_id,
            package_name, package_location;
    
    private PackageState package_state;
    
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
    
    private String user_name;
}
