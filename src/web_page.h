#pragma once

#include "../containers/windows/cairo/windows_container.h"
#include "../containers/windows/cairo/cairo_font.h"
#include "../containers/cairo/cairo_images_cache.h"
#include "tordexhttp.h"
#include <fstream>

class CHTMLViewWnd;

class web_page : public windows_container
{
	CHTMLViewWnd*				m_parent;
	LONG						m_refCount;
public:
	tordex::http				m_http;
	std::string					m_url;
	litehtml::document::ptr		m_doc;
	std::string				m_caption;
	std::string				m_cursor;
	std::string					m_base_path;
	HANDLE						m_hWaitDownload;
	std::string				m_waited_file;
	std::string				m_hash;
	cairo_images_cache			m_images;
public:
	web_page(CHTMLViewWnd* parent);
	virtual ~web_page();

    void load(const std::string& url);
	// encoding: as specified in Content-Type HTTP header
	//   it is NULL for local files or if Content-Type header is not present or Content-Type header doesn't contain "charset="
    void on_document_loaded(const std::string& file, const std::string& encoding, const std::string& realUrl);
    void on_image_loaded(const std::string& file, const std::string& url, bool redraw_only);
    void on_document_error(int status, const std::string& errMsg);
    void on_waited_finished(int status, const std::string& file);
	void add_ref();
	void release();
    void get_url(std::string& url);

	// litehtml::document_container members
	void set_caption(const char* caption) override;
	void set_base_url(const char* base_url) override;
	void import_css(litehtml::string& text, const litehtml::string& url, litehtml::string& baseurl) override;
	void on_anchor_click(const char* url, const litehtml::element::ptr& el) override;
	void set_cursor(const char* cursor) override;
	void load_image(const std::string& src, const std::string& baseurl, bool redraw_on_ready) override;
	void make_url(const std::string& url, std::string basepath, litehtml::string& out) override;

	cairo_surface_t* get_image(const std::string& url) override;
	void get_client_rect(litehtml::position& client) const  override;
private:
    char* load_text_file(const std::string& path, bool is_html, const std::string& defEncoding = "UTF-8", const std::string& forceEncoding = "");
    bool download_and_wait(const std::string& url);
};

enum web_file_type
{
	web_file_document,
	web_file_image_redraw,
	web_file_image_rerender,
	web_file_waited,
};

class web_file : public tordex::http_request
{
    std::string m_file;
    web_page* m_page;
    web_file_type m_type;
    std::ofstream m_ofs;
    LPVOID m_data;
    std::string m_realUrl;
    std::string m_encoding;
public:
    web_file(web_page* page, web_file_type type, LPVOID data = NULL);
    virtual ~web_file();

    virtual void OnFinish(int status, const std::string& errorMsg, const std::string url) override;
    virtual void OnData(const char* data, size_t len, size_t downloaded, size_t total) override;
    virtual void OnHeadersReady() override;

};
