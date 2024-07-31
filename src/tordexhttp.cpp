#include "tordexhttp.h"
#include "httplib.h"

tordex::http::http()
{
}

tordex::http::~http()
{
	stop();
}

bool tordex::http::download_file(const std::string& url, http_request* request)
{
    if (request) {
        if (request->create(url)) {
            m_requests.push_back(request);
            return true;
        }
    }
    return false;
}

void tordex::http::stop()
{
    for (auto& request : m_requests) {
        request->cancel();
    }
    m_requests.clear();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

tordex::http_request::http_request() {
}

tordex::http_request::~http_request() {
	cancel();
}

std::string extractHost(const std::string& url) {
  const std::string prot_end("://");
  auto prot_pos = url.find(prot_end);
  if (prot_pos == std::string::npos) {
    return "";
  }

  auto host_start = prot_pos + prot_end.length();
  auto host_end = url.find('/', host_start);
  if (host_end == std::string::npos) {
    host_end = url.length();
  }

  return url.substr(host_start, host_end - host_start);
}

std::string extractPath(const std::string& url) {
  std::regex url_regex(R"((http|https|ftp)://([^/]+)(/.*)?)");
  std::smatch match;
  if (std::regex_search(url, match, url_regex) && match.size() > 3) {
    return match.str(3);
  }
  return "/";
}

bool tordex::http_request::create(std::string url) {
  m_url = url;
  std::string host = extractHost(url);
  std::string path = extractPath(url);
  if (!m_client) {
    m_client = std::make_shared<httplib::Client>(host);
  }
  httplib::Headers headers = {
      {"User-Agent", "cpp-httplib/0.10"},
      {"Accept", "text/html"}
  };
  //m_client->enable_server_certificate_verification(false);
  auto res = m_client->Get(path, [this](const char* data, size_t size) {
    this->OnData(data, size, m_downloaded_length += size, m_content_length);
    return true;
    });

  if (!res) { 
    OnFinish(-1, "Request failed", m_url);
    return false;
  }

  m_status = res->status;
  m_response_body = res->body;

  OnFinish(m_status, "", m_url);
  return true;
}

void tordex::http_request::cancel()
{
	if(m_client) {
		m_client->stop();
	}
}

void tordex::http_request::OnHeadersReady() {

}