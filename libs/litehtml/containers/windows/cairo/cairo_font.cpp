#include "cairo_font.h"
#include "include/core/SkTypeface.h"
#include "include/core/SkFont.h"
#include "include/core/SkFontMetrics.h"
#include "include/core/SkFontTypes.h"

cairo_font::cairo_font(IMLangFontLink2* fl, HFONT hFont, int size )
{
	init();
	m_font_link = fl;
	if(m_font_link)
	{
		m_font_link->AddRef();
	}
	m_size = size;
	set_font(hFont);
}

cairo_font::cairo_font(IMLangFontLink2* fl, LPCSTR facename, int size, int weight, BOOL italic, BOOL strikeout, BOOL underline )
{
	init();
	m_size = size;
	m_font_link = fl;
	if(m_font_link) {
		m_font_link->AddRef();
	}

	LOGFONT lf;
	ZeroMemory(&lf, sizeof(lf));
	if(!lstrcmpi(facename, "monospace"))
	{
		_tcscpy_s(lf.lfFaceName, LF_FACESIZE, "Courier New");
	} else if(!lstrcmpi(facename, "serif"))
	{
		_tcscpy_s(lf.lfFaceName, LF_FACESIZE, "Times New Roman");
	} else if(!lstrcmpi(facename, "sans-serif"))
	{
		_tcscpy_s(lf.lfFaceName, LF_FACESIZE, "Arial");
	} else if(!lstrcmpi(facename, "fantasy"))
	{
		_tcscpy_s(lf.lfFaceName, LF_FACESIZE, "Impact");
	} else if(!lstrcmpi(facename, "cursive"))
	{
		_tcscpy_s(lf.lfFaceName, LF_FACESIZE, "Comic Sans MS");
	} else
	{
		_tcscpy_s(lf.lfFaceName, LF_FACESIZE, facename);
	}

	lf.lfHeight			= -size;
	lf.lfWeight			= weight;
	lf.lfItalic			= italic;
	lf.lfCharSet		= DEFAULT_CHARSET;
	lf.lfOutPrecision	= OUT_DEFAULT_PRECIS;
	lf.lfClipPrecision	= CLIP_DEFAULT_PRECIS;
	lf.lfQuality		= DEFAULT_QUALITY;
	lf.lfStrikeOut		= strikeout;
	lf.lfUnderline		= underline;

	HFONT fnt = CreateFontIndirect(&lf);
	set_font(fnt);
}

cairo_font::~cairo_font()
{
	if(m_font_face)
	{
		//cairo_font_face_destroy(m_font_face);
	}
	for(size_t i = 0; i < m_linked_fonts.size(); i++)
	{
		if(m_linked_fonts[i]->hFont)
		{
			m_font_link->ReleaseFont(m_linked_fonts[i]->hFont);
		}
		if(m_linked_fonts[i]->font_face)
		{
			//cairo_font_face_destroy(m_linked_fonts[i]->font_face);
		}
	}
	m_linked_fonts.clear();
	if(m_font_link)
	{
		m_font_link->AddRef();
	}
	if(m_hFont)
	{
		DeleteObject(m_hFont);
	}
}

#include "include/core/SkPaint.h"
#include "include/core/SkTextBlob.h"
#include "include/core/SkPixmap.h"

void cairo_font::show_text(SkCanvas* canvas, int x, int y, const char* str ,litehtml::web_color color)
{
	lock();
	text_chunk::vector chunks;
	split_text(str, chunks);

	SkPaint paint;
  paint.setAntiAlias(true);
  paint.setColor(SkColorSetARGB(color.alpha, color.red, color.green, color.blue));
	SkScalar xPos = static_cast<SkScalar>(x);
	SkScalar yPos = static_cast<SkScalar>(y);

	for(size_t i = 0; i < chunks.size(); i++) {
		sk_sp<SkTypeface> typeface = (chunks[i]->font) ? chunks[i]->font->font_face : m_font_face;
		SkFont font(typeface, m_size);
		font.setEdging(SkFont::Edging::kAntiAlias);
		font.setSubpixel(true);

		// 创建 SkTextBlob
		//auto blob = SkTextBlob::MakeFromString(chunks[i]->text, font);
		//canvas->drawTextBlob(blob, xPos, yPos, paint);
		canvas->drawString(chunks[i]->text, xPos, yPos, font, paint);
	}
	unlock();

	if(m_bUnderline)
	{
		int tw = text_width(canvas, chunks);

		lock();
		paint.setStyle(SkPaint::kStroke_Style);
		paint.setStrokeWidth(1);
		canvas->drawLine(xPos, yPos + 1.5f, xPos + static_cast<SkScalar>(tw), yPos + 1.5f, paint);
		unlock();
	}
	if(m_bStrikeOut)
	{
		int tw = text_width(canvas, chunks);

		cairo_font_metrics fm;
		get_metrics(canvas, &fm);

		int ln_y = y - fm.x_height / 2;

		lock();
		paint.setStyle(SkPaint::kStroke_Style);
		paint.setStrokeWidth(1);
		canvas->drawLine(xPos, static_cast<SkScalar>(ln_y) - 0.5f, xPos + static_cast<SkScalar>(tw), static_cast<SkScalar>(ln_y) - 0.5f, paint);
		unlock();
	}

	free_text_chunks(chunks);
}

void cairo_font::split_text( const char* src, text_chunk::vector& chunks )
{
	auto str = cairo_font::utf8_to_wchar(src);
	const wchar_t* str_start = str.c_str();

	int cch = str.length();

	HDC hdc = GetDC(NULL);
	SelectObject(hdc, m_hFont);
	HRESULT hr = S_OK;
	while(cch > 0)
	{
		DWORD dwActualCodePages;
		long cchActual;
		if(m_font_link)
		{
			hr = m_font_link->GetStrCodePages(str.c_str(), cch, m_font_code_pages, &dwActualCodePages, &cchActual);
		} else
		{
			hr = S_FALSE;
		}
		
		if(hr != S_OK)
		{
			break;
		}
		
		text_chunk* chk = new text_chunk;

		int sz = WideCharToMultiByte(CP_UTF8, 0, str.c_str(), cchActual, chk->text, 0, NULL, NULL) + 1;
		chk->text = new CHAR[sz];
		sz = WideCharToMultiByte(CP_UTF8, 0, str.c_str(), cchActual, chk->text, sz, NULL, NULL);
		chk->text[sz] = 0;
		chk->font = NULL;

		if(!(dwActualCodePages & m_font_code_pages))
		{
			for(linked_font::vector::iterator i = m_linked_fonts.begin(); i != m_linked_fonts.end(); i++)
			{
				if((*i)->code_pages == dwActualCodePages)
				{
					chk->font = (*i);
					break;
				}
			}
			if(!chk->font)
			{
				linked_font* lkf = new linked_font;
				lkf->code_pages	= dwActualCodePages;
				lkf->hFont		= NULL;
				m_font_link->MapFont(hdc, dwActualCodePages, 0, &lkf->hFont);
				if (lkf->hFont)
				{
					lkf->font_face = create_font_face(lkf->hFont);
					m_linked_fonts.push_back(lkf);
				}
				else
				{
					delete lkf;
					lkf = NULL;
				}
				chk->font = lkf;
			}
		}

		chunks.push_back(chk);

		cch -= cchActual;
		str += cchActual;
	}

	if(hr != S_OK)
	{
		text_chunk* chk = new text_chunk;

		int sz = WideCharToMultiByte(CP_UTF8, 0, str.c_str(), -1, chk->text, 0, NULL, NULL) + 1;
		chk->text = new CHAR[sz];
		sz = WideCharToMultiByte(CP_UTF8, 0, str.c_str(), -1, chk->text, sz, NULL, NULL);
		chk->text[sz] = 0;
		chk->font = NULL;
		chunks.push_back(chk);
	}

	ReleaseDC(NULL, hdc);
}

void cairo_font::free_text_chunks( text_chunk::vector& chunks )
{
	for(size_t i = 0; i < chunks.size(); i++)
	{
		delete chunks[i];
	}
	chunks.clear();
}

#include "include/core/SkTypeface.h"
#include "include/ports/SKTypeface_win.h"
#include "include/core/SkFontMgr.h"

sk_sp<SkTypeface> cairo_font::create_font_face( HFONT fnt )
{
	LOGFONT lf;
	GetObject(fnt, sizeof(LOGFONT), &lf);

	sk_sp<SkFontMgr> mgr = SkFontMgr_New_DirectWrite();
	SkString familyName(lf.lfFaceName);

	SkFontStyle fontStyle;
	if (lf.lfWeight >= FW_BOLD) {
		if (lf.lfItalic) {
			fontStyle = SkFontStyle::BoldItalic();
		} else {
			fontStyle = SkFontStyle::Bold();
		}
	} else {
		if (lf.lfItalic) {
			fontStyle = SkFontStyle::Italic();
		} else {
			fontStyle = SkFontStyle::Normal();
		}
	}

	// 创建 SkTypeface
	sk_sp<SkTypeface> typeface = mgr->matchFamilyStyle(familyName.c_str(), fontStyle);

	return typeface;
}

int cairo_font::text_width(SkCanvas* canvas, const char* str )
{
	text_chunk::vector chunks;
	split_text(str, chunks);

	int ret = text_width(canvas, chunks);

	free_text_chunks(chunks);

	return (int) ret;
}

int cairo_font::text_width(SkCanvas* canvas, text_chunk::vector& chunks )
{
	lock();

	SkFont font(m_font_face, m_size);
	double ret = 0;

	for (size_t i = 0; i < chunks.size(); ++i) {
		sk_sp<SkTypeface> typeface = (chunks[i]->font) ? chunks[i]->font->font_face : m_font_face;
		font.setTypeface(typeface);

		SkRect bounds;
		int width = font.measureText(chunks[i]->text, strlen(chunks[i]->text), SkTextEncoding::kUTF8, &bounds);
		ret += width;//bounds.width();
	}

	unlock();

	return static_cast<int>(ret);
}

void cairo_font::get_metrics(SkCanvas* canvas, cairo_font_metrics* fm )
{
	lock();
	SkFont font(m_font_face, m_size);
	font.setEdging(SkFont::Edging::kAntiAlias);
	font.setSubpixel(true);

	SkFontMetrics metrics;
	font.getMetrics(&metrics);

	fm->ascent = static_cast<int>(-metrics.fAscent);
	fm->descent = static_cast<int>(metrics.fDescent);
	fm->height = static_cast<int>(-metrics.fAscent + metrics.fDescent + metrics.fLeading);

	SkRect bounds;
	int width = font.measureText("x", 1, SkTextEncoding::kUTF8, &bounds);
	fm->x_height = width;

	unlock();
}


void cairo_font::set_font(HFONT hFont) {
	clear();

	m_hFont = hFont;
	m_font_face = create_font_face(m_hFont);
	m_font_code_pages = 0;
	if (m_font_link)
	{
		HDC hdc = GetDC(NULL);
		SelectObject(hdc, m_hFont);
		m_font_link->GetFontCodePages(hdc, m_hFont, &m_font_code_pages);
		ReleaseDC(NULL, hdc);
	}
	LOGFONT lf;
	GetObject(m_hFont, sizeof(LOGFONT), &lf);
	m_bUnderline = lf.lfUnderline;
	m_bStrikeOut = lf.lfStrikeOut;
}

void cairo_font::clear()
{
	if(m_font_face)
	{
		//cairo_font_face_destroy(m_font_face);
		m_font_face = NULL;
	}
	for(size_t i = 0; i < m_linked_fonts.size(); i++)
	{
		if(m_linked_fonts[i]->hFont && m_font_link)
		{
			m_font_link->ReleaseFont(m_linked_fonts[i]->hFont);
		}
		if(m_linked_fonts[i]->font_face)
		{
			//cairo_font_face_destroy(m_linked_fonts[i]->font_face);
		}
	}
	m_linked_fonts.clear();
	if(m_hFont)
	{
		DeleteObject(m_hFont);
		m_hFont = NULL;
	}
}

void cairo_font::init()
{
	m_hFont				= NULL;
	m_font_face			= NULL;
	m_font_link			= NULL;
	m_font_code_pages	= 0;
	m_size				= 0;
	m_bUnderline		= FALSE;
	m_bStrikeOut		= FALSE;
}

std::wstring cairo_font::utf8_to_wchar(const std::string& src )
{
	if (src.empty()) return std::wstring();

	int len = (int) src.size();
	wchar_t* str = new wchar_t[len + 1];
	MultiByteToWideChar(CP_UTF8, 0, src.c_str(), -1, str, len + 1);
	std::wstring ret(str);
	delete str;
	return ret;
}

std::string cairo_font::wchar_to_utf8(const std::wstring& src)
{
	if(src.empty()) return std::string();

	int len = WideCharToMultiByte(CP_UTF8, 0, src.c_str(), -1, NULL, 0, NULL, NULL);
	char* str = new char[len];
	WideCharToMultiByte(CP_UTF8, 0, src.c_str(), -1, str, len, NULL, NULL);
	std::string ret(str);
	delete str;
	return ret;
}
