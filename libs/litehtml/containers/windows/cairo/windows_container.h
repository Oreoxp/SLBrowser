#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <mlang.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <vector>
#include "cairo.h"
#include "cairo-win32.h"
#include <litehtml.h>
#include <dib.h>
#include <txdib.h>
#include "../../libs/litehtml/containers/cairo/container_cairo.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkSurface.h"

class windows_container :	public container_cairo
{
protected:
	cairo_surface_t*			m_temp_surface;
	cairo_t*					    m_temp_cr;
	IMLangFontLink2*			m_font_link;
	SkCanvas*             m_temp_canvas;
	sk_sp<SkSurface>      m_surface;
public:
	windows_container(void);
	virtual ~windows_container(void);

	litehtml::uint_ptr			create_font(const char* faceName, int size, int weight, litehtml::font_style italic, unsigned int decoration, litehtml::font_metrics* fm) override;
	void						delete_font(litehtml::uint_ptr hFont) override;
	int							text_width(const char* text, litehtml::uint_ptr hFont) override;
	void						draw_text(litehtml::uint_ptr hdc, const char* text, litehtml::uint_ptr hFont, litehtml::web_color color, const litehtml::position& pos) override;
    litehtml::string			resolve_color(const litehtml::string& color) const override;

  cairo_surface_t* get_image(const std::string& url)  override;
  CTxDIB* get_image_ctxdib(const std::string& url) override;
	double get_screen_dpi() const override;
	int get_screen_width() const override;
	int get_screen_height() const override;
};
