
#include "httplib.h"
#include <stdlib.h>
#include <vector>
#include <string>
namespace tordex
{
class http_request
{
    friend class http;
protected:
    std::string m_url;
    LONG m_refCount = 1;
    std::shared_ptr<httplib::Client> m_client;
    ULONG64 m_content_length;
    ULONG64 m_downloaded_length;
    int m_status;
    httplib::Headers m_headers;
    std::string m_response_body;
public:
    http_request();
    virtual ~http_request();

    virtual void OnFinish(int status, const std::string& errorMsg) = 0;
    virtual void OnData(const char* data, size_t len, size_t downloaded, size_t total) = 0;
    virtual void OnHeadersReady();

    bool	create(std::string url);
    void	cancel();
    void	add_ref();
    void	release();

protected:
    static void* read_callback(const char* data, size_t size, size_t nmemb, void* userdata);
    static void* write_callback(const char* data, size_t size, size_t nmemb, void* userdata);
};
}

tordex::http_request::http_request() {
}

tordex::http_request::~http_request() {
	cancel();
}

bool tordex::http_request::create(std::string url) {
	m_url	= url;
    if(!m_client) {
        m_client = std::make_shared<httplib::Client>(url);
    }
    auto res = m_client->Get(url.c_str(), [this](const char* data, size_t size) {
        this->OnData(data, size, m_downloaded_length += size, m_content_length);
        return true;
    });

    if (!res) {
        OnFinish(-1, "Request failed");
        return false;
    }

    m_status = res->status;
    m_response_body = res->body;

    OnFinish(m_status, "");
	return true;
}

void tordex::http_request::cancel()
{
	if(m_client) {
		m_client->stop();
	}
}

int main(){
    std::cout << "test http request" << std::endl;
}