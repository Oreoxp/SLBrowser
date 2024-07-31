#include "globals.h"
#include "web_page.h"
#include "HtmlViewWnd.h"


web_page::web_page(CHTMLViewWnd* parent)
{
	m_refCount		= 1;
	m_parent		= parent;
}

web_page::~web_page()
{
	m_http.stop();
}

void web_page::set_caption( const char* caption )
{
	m_caption = caption;
}

void web_page::set_base_url( const char* base_url )
{
	if(base_url)
	{
		if(PathIsRelative(base_url) && !PathIsURL(base_url))
		{
			make_url(base_url, m_url.c_str(), m_base_path);
		} else
		{
			m_base_path = base_url;
		}
	} else
	{
		m_base_path = m_url;
	}
}

void web_page::import_css( litehtml::string& text, const litehtml::string& url, litehtml::string& baseurl )
{
	std::string css_url;
	make_url(url.c_str(), baseurl.c_str(), css_url);

	if(download_and_wait(css_url))
	{
		LPSTR css = load_text_file(m_waited_file, false, "UTF-8");
		if(css)
		{
			baseurl = css_url;
			text = css;
		}
	}
}

void web_page::on_anchor_click( const char* url, const litehtml::element::ptr& el )
{
	std::string anchor;
	make_url(url, m_base_path.c_str(), anchor);
	m_parent->open(anchor.c_str());
}

void web_page::set_cursor( const char* cursor )
{
	m_cursor = cursor;
}

void web_page::load_image(const std::string& src, const std::string& baseurl, bool redraw_on_ready)
{
	std::string url;
	make_url(src, baseurl, url);
	if (m_images.reserve(url))
	{
		if (PathIsURL(url.c_str()))
		{
			if (redraw_on_ready)
			{
				m_http.download_file(url, new web_file(this, web_file_image_redraw));
			}
			else
			{
				m_http.download_file(url, new web_file(this, web_file_image_rerender));
			}
		}
		else
		{
			on_image_loaded(url, url, redraw_on_ready);
		}
	}
}

void web_page::make_url(const std::string& url, std::string basepath, litehtml::string& out)
{
	if (PathIsRelative(url.c_str()) && !PathIsURL(url.c_str()))
	{
		if (basepath.empty())
		{
			basepath = m_base_path;
		}
		DWORD dl = (DWORD)url.length() + (DWORD)basepath.length() + 1;
		LPSTR abs_url = new CHAR[dl];
		HRESULT res = UrlCombine(basepath.c_str(), url.c_str(), abs_url, &dl, 0);
		if (res == E_POINTER)
		{
			delete abs_url;
			abs_url = new CHAR[dl + 1];
			if (UrlCombine(basepath.c_str(), url.c_str(), abs_url, &dl, 0) == S_OK)
			{
				out = abs_url;
			}
		}
		else if (res == S_OK)
		{
			out = abs_url;
		}
		delete abs_url;
	}
	else
	{
		if (PathIsURL(url.c_str()))
		{
			out = url;
		}
		else
		{
			DWORD dl = (DWORD)url.length() + 1;
			LPSTR abs_url = new CHAR[dl];
			HRESULT res = UrlCreateFromPath(url.c_str(), abs_url, &dl, 0);
			if (res == E_POINTER)
			{
				delete abs_url;
				abs_url = new CHAR[dl + 1];
				if (UrlCreateFromPath(url.c_str(), abs_url, &dl, 0) == S_OK)
				{
					out = abs_url;
				}
			}
			else if (res == S_OK)
			{
				out = abs_url;
			}
			delete abs_url;
		}
	}
	if (out.substr(0, 8) == "file:///")
	{
		out.erase(5, 1);
	}
	if (out.substr(0, 7) == "file://")
	{
		out.erase(0, 7);
	}
}

void web_page::load(const std::string& url )
{
	m_url = url;
	m_base_path	= m_url;
	if(PathIsURLA(url.c_str()))
	{
		m_http.download_file( url, new web_file(this, web_file_document) );
	} else
	{
		on_document_loaded(url, "", "");
	}
}

void web_page::on_document_loaded(const std::string& file, const std::string& encoding, const std::string& realUrl)
{
	if (!realUrl.empty())
	{
		m_url = realUrl;
	}

	char* html_text = load_text_file(file, true, "UTF-8", encoding);

	if(!html_text)
	{
		LPCSTR txt = "<h1>Something Wrong</h1>";
		html_text = new char[lstrlenA(txt) + 1];
		lstrcpyA(html_text, txt);
	}

	m_doc = litehtml::document::createFromString(html_text, this);
	delete html_text;

	PostMessage(m_parent->wnd(), WM_PAGE_LOADED, 0, 0);
}

void web_page::on_document_error(int status, const std::string& errMsg)
{
	std::string txt = "<h1>Something Wrong</h1>";
	if(!errMsg.empty())
	{
		auto errMsg_utf8 = errMsg;
		txt += "<p>";
		txt += errMsg_utf8;
		txt += "</p>";
	}
	m_doc = litehtml::document::createFromString(txt.c_str(), this);

	PostMessage(m_parent->wnd(), WM_PAGE_LOADED, 0, 0);
}

void web_page::on_image_loaded(const std::string& file, const std::string& url, bool redraw_only)
{
	CTxDIB img;
	if(img.load(file))
	{
		cairo_surface_t* surface = dib_to_surface(img);
		m_images.add_image(url, surface);
		if(m_doc)
		{
			PostMessage(m_parent->wnd(), WM_IMAGE_LOADED, (WPARAM) (redraw_only ? 1 : 0), 0);
		}
	}
}
bool web_page::download_and_wait(const std::string& url)
{
	if(PathIsURLA(url.c_str()))
	{
		m_waited_file = "";
		m_hWaitDownload = CreateEvent(NULL, TRUE, FALSE, NULL);
		if(m_http.download_file( url, new web_file(this, web_file_waited) ))
		{
			WaitForSingleObject(m_hWaitDownload, INFINITE);
			CloseHandle(m_hWaitDownload);
			if(m_waited_file.empty())
			{
				return FALSE;
			}
			return TRUE;
		}
	} else
	{
		m_waited_file = url;
		return TRUE;
	}
	return FALSE;
}

void web_page::on_waited_finished(int status, const std::string& file)
{
	if(status != 200) {
		m_waited_file = "";
	} else {
		m_waited_file = file;
	}
	SetEvent(m_hWaitDownload);
}

cairo_surface_t* web_page::get_image(const std::string& url)
{
	return m_images.get_image(url);
}

void web_page::get_client_rect( litehtml::position& client ) const
{
	m_parent->get_client_rect(client);
}

void web_page::add_ref()
{
	InterlockedIncrement(&m_refCount);
}

void web_page::release()
{
	LONG lRefCount;
	lRefCount = InterlockedDecrement(&m_refCount);
	if (lRefCount == 0)
	{
		delete this;
	}
}

void web_page::get_url(std::string& url )
{
	url = m_url;
	if(!m_hash.empty())
	{
		url.append("#");
		url.append(m_hash);
	}
}

char* web_page::load_text_file(const std::string& path, bool is_html, const std::string& defEncoding, const std::string& forceEncoding)
{
	char* ret = NULL;

	HANDLE fl = CreateFile(path.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(fl != INVALID_HANDLE_VALUE)
	{
		DWORD size = GetFileSize(fl, NULL);
		ret = new char[size + 1];

		DWORD cbRead = 0;
		if(size >= 3)
		{
			ReadFile(fl, ret, 3, &cbRead, NULL);
			if(ret[0] == '\xEF' && ret[1] == '\xBB' && ret[2] == '\xBF')
			{
				ReadFile(fl, ret, size - 3, &cbRead, NULL);
				ret[cbRead] = 0;
			} else
			{
				ReadFile(fl, ret + 3, size - 3, &cbRead, NULL);
				ret[cbRead + 3] = 0;
			}
		}
		CloseHandle(fl);
	}

	// try to convert encoding
	if(is_html)
	{
		std::string encoding;
		if (!forceEncoding.empty())
		{
			encoding = forceEncoding;
		}
		else
		{
			char* begin = StrStrIA((LPSTR)ret, "<meta");
			while (begin && encoding.empty())
			{
				char* end = StrStrIA(begin, ">");
				char* s1 = StrStrIA(begin, "Content-Type");
				if (s1 && s1 < end)
				{
					s1 = StrStrIA(begin, "charset");
					if (s1)
					{
						s1 += strlen("charset");
						while (!isalnum(s1[0]) && s1 < end)
						{
							s1++;
						}
						while ((isalnum(s1[0]) || s1[0] == '-') && s1 < end)
						{
							encoding += s1[0];
							s1++;
						}
					}
				}
				if (encoding.empty())
				{
					begin = StrStrIA(begin + strlen("<meta"), "<meta");
				}
			}

			if (encoding.empty() && !defEncoding.empty())
			{
				encoding = defEncoding;
			}
		}

		if(!encoding.empty())
		{
			if(!StrCmpI(encoding.c_str(), "UTF-8"))
			{
				encoding.clear();
			}
		}

		if(!encoding.empty())
		{
			CoInitialize(NULL);

			IMultiLanguage* ml = NULL;
			HRESULT hr = CoCreateInstance(CLSID_CMultiLanguage, NULL, CLSCTX_INPROC_SERVER, IID_IMultiLanguage, (LPVOID*) &ml);	

			MIMECSETINFO charset_src = {0};
			MIMECSETINFO charset_dst = {0};

			BSTR bstrCharSet = SysAllocString(cairo_font::utf8_to_wchar(encoding).c_str());
			ml->GetCharsetInfo(bstrCharSet, &charset_src);
			SysFreeString(bstrCharSet);

			bstrCharSet = SysAllocString(L"utf-8");
			ml->GetCharsetInfo(bstrCharSet, &charset_dst);
			SysFreeString(bstrCharSet);

			DWORD dwMode = 0;
			UINT  szDst = (UINT) strlen((LPSTR) ret) * 4;
			LPSTR dst = new char[szDst];

			if(ml->ConvertString(&dwMode, charset_src.uiInternetEncoding, charset_dst.uiInternetEncoding, (LPBYTE) ret, NULL, (LPBYTE) dst, &szDst) == S_OK)
			{
				dst[szDst] = 0;
				delete ret;
				ret = dst;
			} else
			{
				delete dst;
			}
			CoUninitialize();
		}
	}

	return ret;
}

//////////////////////////////////////////////////////////////////////////

web_file::web_file(web_page* page, web_file_type type, LPVOID data)
    : m_page(page), m_type(type), m_data(data) {
    char path[MAX_PATH];
    GetTempPathA(MAX_PATH, path);
		char path2[MAX_PATH];
    GetTempFileNameA(path, "lbr", 0, path2);
		m_file = path2;
    m_ofs.open(m_file, std::ios::binary);

    m_page->add_ref();
}

web_file::~web_file() {
    if (m_ofs.is_open()) {
        m_ofs.close();
    }
    if (m_type != web_file_waited) {
        std::remove(m_file.c_str());
    }
    if (m_page) {
        m_page->release();
    }
}

void web_file::OnFinish(int status, const std::string& errorMsg, const std::string url) {
    if (m_ofs.is_open()) {
        m_ofs.close();
    }
    if (status != 200) {
        switch (m_type) {
        case web_file_document:
            m_page->on_document_error(status, errorMsg);
            break;
        case web_file_waited:
            m_page->on_waited_finished(status, m_file);
            break;
        }
    } else {
        switch (m_type) {
        case web_file_document:
            m_page->on_document_loaded(m_file, m_encoding, url);
            break;
        case web_file_image_redraw:
            m_page->on_image_loaded(m_file, url, true);
            break;
        case web_file_image_rerender:
            m_page->on_image_loaded(m_file, url, false);
            break;
        case web_file_waited:
            m_page->on_waited_finished(status, m_file);
            break;
        }
    }
}

void web_file::OnData(const char* data, size_t len, size_t downloaded, size_t total) {
    if (m_ofs.is_open()) {
        m_ofs.write(data, len);
    }
}


void web_file::OnHeadersReady()
{/*
	WCHAR buf[2048];
	DWORD len = sizeof(buf);
	if (WinHttpQueryOption(m_hRequest, WINHTTP_OPTION_URL, buf, &len))
	{
		m_realUrl = buf;
	}
	len = sizeof(buf);
	if (WinHttpQueryHeaders(m_hRequest, WINHTTP_QUERY_CONTENT_TYPE, NULL, buf, &len, NULL))
	{
		WCHAR* pos = wcsstr(buf, L"charset=");
		if (pos)
		{
			m_encoding = pos + wcslen(L"charset=");
		}
	}*/
}
