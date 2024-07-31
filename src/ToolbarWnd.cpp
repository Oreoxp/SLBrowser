#include "globals.h"
#include "ToolbarWnd.h"
#include <WindowsX.h>
#include "BrowserWnd.h"
#include "el_omnibox.h"

#include "include/core/SkCanvas.h"
#include "include/core/SkPaint.h"
#include "include/core/SkSurface.h"
#include "include/core/SkImageInfo.h"
#include "include/encode/SkPngEncoder.h"
#include "include/core/SkStream.h"
#include "include/core/SkBitmap.h"
#include <TxDIB.h>

static int count = 0;
static int dib_count = 0;

CToolbarWnd::CToolbarWnd( HINSTANCE hInst, CBrowserWnd* parent )
{
	m_inCapture = FALSE;
	m_omnibox	= nullptr;
	m_parent	= parent;
	m_hInst		= hInst;
	m_hWnd		= NULL;

	WNDCLASS wc;
	if(!GetClassInfo(m_hInst, TOOLBARWND_CLASS, &wc))
	{
		ZeroMemory(&wc, sizeof(wc));
		wc.style          = CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc    = (WNDPROC)CToolbarWnd::WndProc;
		wc.cbClsExtra     = 0;
		wc.cbWndExtra     = 0;
		wc.hInstance      = m_hInst;
		wc.hIcon          = NULL;
		wc.hCursor        = NULL/*LoadCursor(NULL, IDC_ARROW)*/;
		wc.hbrBackground  = (HBRUSH) (COLOR_WINDOW + 1);
		wc.lpszMenuName   = NULL;
		wc.lpszClassName  = TOOLBARWND_CLASS;

		RegisterClass(&wc);
	}
}
CToolbarWnd::~CToolbarWnd(void)
{
	if (m_omnibox)
	{
		m_omnibox = nullptr;
	}
}

void SaveDIBToPNG(simpledib::dib* dib, const char* filename) {
	if (!dib) {
		printf("Invalid DIB pointer.\n");
		return;
	}

	int width = dib->width();
	int height = dib->height();
	unsigned char* pixels = (unsigned char*)dib->bits();

	if (!pixels) {
		printf("Failed to get DIB bits.\n");
		return;
	}

	// Create SkImageInfo and SkPixmap
	SkImageInfo info = SkImageInfo::MakeN32Premul(width, height);
	SkPixmap pixmap(info, pixels, width * 4);

	// Encode to PNG using SkPngEncoder
	SkFILEWStream output(filename);
	if (!output.isValid()) {
		printf("Failed to create output stream.\n");
		return;
	}

	SkPngEncoder::Options options;
	if (!SkPngEncoder::Encode(&output, pixmap, options)) {
		printf("Failed to encode PNG.\n");
		return;
	}

	printf("Saved DIB to %s successfully.\n", filename);
}

LRESULT CALLBACK CToolbarWnd::WndProc( HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam )
{
	CToolbarWnd* pThis = NULL;
	if(IsWindow(hWnd))
	{
		pThis = (CToolbarWnd*)GetProp(hWnd, TEXT("toolbar_this"));
		if(pThis && pThis->m_hWnd != hWnd)
		{
			pThis = NULL;
		}
	}

	if(pThis || uMessage == WM_CREATE)
	{
		switch (uMessage)
		{
		case WM_EDIT_CAPTURE:
			if (wParam)
			{
				SetCapture(hWnd);
				pThis->m_inCapture = TRUE;
			}
			else
			{
				ReleaseCapture();
				pThis->m_inCapture = FALSE;
			}
			break;
		case WM_EDIT_ACTIONKEY:
			switch (wParam)
			{
			case VK_RETURN:
				{
					std::string url = pThis->m_omnibox->get_url();
					pThis->m_omnibox->select_all();
					pThis->m_parent->open(url.c_str());
				}
				break;
			}
			return 0;
		case WM_OMNIBOX_CLICKED:
			pThis->OnOmniboxClicked();
			break;
		case WM_UPDATE_CONTROL:
			{
				LPRECT rcDraw = (LPRECT)lParam;
				InvalidateRect(hWnd, rcDraw, FALSE);
			}
			break;
		case WM_SETCURSOR:
			pThis->update_cursor();
			break;
		case WM_ERASEBKGND:
			return TRUE;
		case WM_CREATE:
			{
				LPCREATESTRUCT lpcs = (LPCREATESTRUCT)lParam;
				pThis = (CToolbarWnd*)(lpcs->lpCreateParams);
				SetProp(hWnd, TEXT("toolbar_this"), (HANDLE) pThis);
				pThis->m_hWnd = hWnd;
				pThis->OnCreate();
			}
			break;
		case WM_PAINT:
			{
				PAINTSTRUCT ps;
				HDC hdc = BeginPaint(hWnd, &ps);

				simpledib::dib dib;

				dib.beginPaint(hdc, &ps.rcPaint);
				pThis->OnPaint(&dib, &ps.rcPaint, hWnd);
				dib.draw(hdc, 0, 0);

				std::string str = "test/11_" + std::to_string(dib_count) + ".png";
				SaveDIBToPNG(&dib, str.c_str());
				dib_count++;
				dib.endPaint();

				EndPaint(hWnd, &ps);
			}
			return 0;
		case WM_KILLFOCUS:
			if (pThis->m_omnibox && pThis->m_omnibox->have_focus())
			{
				pThis->m_omnibox->KillFocus();
			}
			break;
		case WM_SIZE:
			pThis->OnSize(LOWORD(lParam), HIWORD(lParam));
			return 0;
		case WM_DESTROY:
			RemoveProp(hWnd, TEXT("toolbar_this"));
			pThis->OnDestroy();
			delete pThis;
			return 0;
		case WM_MOUSEMOVE:
			{
				TRACKMOUSEEVENT tme;
				ZeroMemory(&tme, sizeof(TRACKMOUSEEVENT));
				tme.cbSize = sizeof(TRACKMOUSEEVENT);
				tme.dwFlags		= TME_QUERY;
				tme.hwndTrack	= hWnd;
				TrackMouseEvent(&tme);
				if(!(tme.dwFlags & TME_LEAVE))
				{
					tme.dwFlags		= TME_LEAVE;
					tme.hwndTrack	= hWnd;
					TrackMouseEvent(&tme);
				}
				pThis->OnMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			}
			return 0;
		case WM_MOUSELEAVE:
			pThis->OnMouseLeave();
			return 0;
		case WM_LBUTTONDOWN:
			pThis->OnLButtonDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			return 0;
		case WM_LBUTTONUP:
			pThis->OnLButtonUp(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			return 0;
		case WM_KEYDOWN:
			if (pThis->m_omnibox && pThis->m_omnibox->have_focus())
			{
				if (pThis->m_omnibox->OnKeyDown(wParam, lParam))
				{
					return 0;
				}
			}
			break;
		case WM_KEYUP:
			if (pThis->m_omnibox && pThis->m_omnibox->have_focus())
			{
				if (pThis->m_omnibox->OnKeyUp(wParam, lParam))
				{
					return 0;
				}
			}
			break;
		case WM_CHAR:
			if (pThis->m_omnibox && pThis->m_omnibox->have_focus())
			{
				if (pThis->m_omnibox->OnChar(wParam, lParam))
				{
					return 0;
				}
			}
			break;
		}
	}

	return DefWindowProc(hWnd, uMessage, wParam, lParam);
}

void CToolbarWnd::render_toolbar(int width)
{
	if (m_doc)
	{
		m_doc->render(width);
		m_omnibox->update_position();
	}
}

void CToolbarWnd::update_cursor()
{
	LPCSTR defArrow = IDC_ARROW;

	if (m_cursor == "pointer")
	{
		SetCursor(LoadCursor(NULL, IDC_HAND));
	}
	else if (m_cursor == "text")
	{
		SetCursor(LoadCursor(NULL, IDC_IBEAM));
	}
	else
	{
		SetCursor(LoadCursor(NULL, defArrow));
	}
}

void CToolbarWnd::OnCreate()
{

}

void CToolbarWnd::OnPaint(simpledib::dib* dib, LPRECT rcDraw, HWND hWnd) {
	if (m_doc) {
		// 创建Skia绘图表面
    int width = dib->width();
    int height = dib->height();
    unsigned char* bits = (unsigned char*)dib->bits();

    // 创建 SkImageInfo 和 SkSurface
    SkImageInfo info = SkImageInfo::MakeN32Premul(width, height);
    SkPixmap pixmap(info, bits, width * 4);
    sk_sp<SkSurface> surface = SkSurfaces::WrapPixels(info, bits, sizeof(uint32_t) * width);

		if (surface) {
			SkCanvas* canvas = surface->getCanvas();

			// 获取窗口原点
			POINT pt;
			GetWindowOrgEx(dib->hdc(), &pt);
			if (pt.x != 0 || pt.y != 0) {
				canvas->translate(-pt.x, -pt.y);
			}

			SkPaint paint;
			paint.setColor(SK_ColorYELLOW);
      canvas->drawRect(SkRect::MakeWH(width, height), paint);

			litehtml::position clip(rcDraw->left, rcDraw->top, rcDraw->right - rcDraw->left, rcDraw->bottom - rcDraw->top);

      std::string path = std::string("test/pre_toolbar_test_output_") + std::to_string(count) + ".png";
      SkFILEWStream output(path.c_str());

			std::string dibpath = "test/11_" + std::to_string(dib_count) + ".png";
			dib_count++;
			SaveDIBToPNG(dib, dibpath.c_str());
      if (surface->peekPixels(&pixmap)) {
        if (!SkPngEncoder::Encode(&output, pixmap, {})) {
          printf("en code error!");
        }
      }

			canvas->save();
			m_doc->draw((litehtml::uint_ptr)canvas, 0, 0, &clip);
			canvas->restore();

			surface->readPixels(pixmap, 0, 0);

			std::string dibpath2 = "test/11_" + std::to_string(dib_count) + ".png";
			dib_count++;
			SaveDIBToPNG(dib, dibpath2.c_str());
			path = std::string("test/end_toolbar_test_output_") + std::to_string(count) + ".png";
			SkFILEWStream output2(path.c_str());
      count++;

			if (surface->peekPixels(&pixmap)) {
				if (!SkPngEncoder::Encode(&output2, pixmap, {})) {
					printf("en code error!");
				}
			}
			// 释放Skia表面
			surface->unref();

		}
	}
}

void CToolbarWnd::OnSize( int width, int height )
{

}

void CToolbarWnd::OnDestroy()
{

}

void CToolbarWnd::create( int x, int y, int width, HWND parent )
{
	LPSTR html = NULL;

	HRSRC hResource = ::FindResource(m_hInst, "toolbar.html", RT_HTML);
	if(hResource)
	{
		DWORD imageSize = ::SizeofResource(m_hInst, hResource);
		if(imageSize)
		{
			LPCSTR pResourceData = (LPCSTR) ::LockResource(::LoadResource(m_hInst, hResource));
			if(pResourceData)
			{
				html = new CHAR[imageSize + 1];
				lstrcpynA(html, pResourceData, imageSize);
				html[imageSize] = 0;
			}
		}
	}
	m_hWnd = CreateWindow(TOOLBARWND_CLASS, "toolbar", WS_CHILD | WS_VISIBLE, x, y, width, 1, parent, NULL, m_hInst, (LPVOID) this);

	m_doc = litehtml::document::createFromString(html, this, "html,div,body { display: block; } head,style { display: none; }", "", 1);
	delete html;
	render_toolbar(width);
	MoveWindow(m_hWnd, x, y, width, m_doc->height(), TRUE);
}

cairo_surface_t* CToolbarWnd::get_image(const std::string& url)
{
	CTxDIB img;
	if (img.load(FindResource(m_hInst, url.c_str(), RT_HTML), m_hInst))
	{
		return dib_to_surface(img);
	}

	return nullptr;
}

CTxDIB* CToolbarWnd::get_image_ctxdib(const std::string& url) {
  CTxDIB* img = new CTxDIB;
  if (img->load(FindResource(m_hInst, url.c_str(), RT_HTML), m_hInst))
  {
    return img;
  }

  return nullptr;
}

void CToolbarWnd::set_caption( const char* caption )
{

}

void CToolbarWnd::set_base_url( const char* base_url )
{

}

void CToolbarWnd::link(std::shared_ptr<litehtml::document>& doc, litehtml::element::ptr el)
{

}

int CToolbarWnd::set_width( int width )
{
	if(m_doc)
	{
		render_toolbar(width);
		
		return m_doc->height();
	}
	return 0;
}

void CToolbarWnd::on_page_loaded(LPCSTR url)
{
	if (m_omnibox)
	{
		m_omnibox->set_url(url);
	}
}

void CToolbarWnd::OnMouseMove(int x, int y)
{
	if(m_doc)
	{
		BOOL process = TRUE;
		if (m_omnibox)
		{
			m_omnibox->OnMouseMove(x, y);
		}
		if (!m_inCapture)
		{
			litehtml::position::vector redraw_boxes;
			if (m_doc->on_mouse_over(x, y, x, y, redraw_boxes))
			{
				for (litehtml::position::vector::iterator box = redraw_boxes.begin(); box != redraw_boxes.end(); box++)
				{
					RECT rcRedraw;
					rcRedraw.left = box->left();
					rcRedraw.right = box->right();
					rcRedraw.top = box->top();
					rcRedraw.bottom = box->bottom();
					InvalidateRect(m_hWnd, &rcRedraw, TRUE);
				}
				UpdateWindow(m_hWnd);
			}
		}
	}
	update_cursor();
}

void CToolbarWnd::OnMouseLeave()
{
	if(m_doc)
	{
		litehtml::position::vector redraw_boxes;
		if(m_doc->on_mouse_leave(redraw_boxes))
		{
			for(litehtml::position::vector::iterator box = redraw_boxes.begin(); box != redraw_boxes.end(); box++)
			{
				RECT rcRedraw;
				rcRedraw.left	= box->left();
				rcRedraw.right	= box->right();
				rcRedraw.top	= box->top();
				rcRedraw.bottom	= box->bottom();
				InvalidateRect(m_hWnd, &rcRedraw, TRUE);
			}
			UpdateWindow(m_hWnd);
		}
	}
}

void CToolbarWnd::OnOmniboxClicked()
{
	SetFocus(m_hWnd);
	m_omnibox->SetFocus();
}

void CToolbarWnd::OnLButtonDown(int x, int y)
{
	if(m_doc)
	{
		BOOL process = TRUE;
		if (m_omnibox && m_omnibox->OnLButtonDown(x, y))
		{
			process = FALSE;
		}
		if (process && !m_inCapture)
		{
			litehtml::position::vector redraw_boxes;
			if (m_doc->on_lbutton_down(x, y, x, y, redraw_boxes))
			{
				for (litehtml::position::vector::iterator box = redraw_boxes.begin(); box != redraw_boxes.end(); box++)
				{
					RECT rcRedraw;
					rcRedraw.left = box->left();
					rcRedraw.right = box->right();
					rcRedraw.top = box->top();
					rcRedraw.bottom = box->bottom();
					InvalidateRect(m_hWnd, &rcRedraw, TRUE);
				}
				UpdateWindow(m_hWnd);
			}
		}
	}
}

void CToolbarWnd::OnLButtonUp( int x, int y )
{
	if(m_doc)
	{
		BOOL process = TRUE;
		if (m_omnibox && m_omnibox->OnLButtonUp(x, y))
		{
			process = FALSE;
		}
		if (process && !m_inCapture)
		{
			litehtml::position::vector redraw_boxes;
			if (m_doc->on_lbutton_up(x, y, x, y, redraw_boxes))
			{
				for (litehtml::position::vector::iterator box = redraw_boxes.begin(); box != redraw_boxes.end(); box++)
				{
					RECT rcRedraw;
					rcRedraw.left = box->left();
					rcRedraw.right = box->right();
					rcRedraw.top = box->top();
					rcRedraw.bottom = box->bottom();
					InvalidateRect(m_hWnd, &rcRedraw, TRUE);
				}
				UpdateWindow(m_hWnd);
			}
		}
	}
}

struct
{
	LPCSTR	name;
	LPCSTR	url;
} g_bookmarks[] = 
{
	{"Wiki: Alexei Navalny",	"https://en.wikipedia.org/wiki/Alexei_Navalny?useskin=vector"},
	{"litehtml website",		"http://www.litehtml.com/"},
	{"True Launch Bar",		"http://www.truelaunchbar.com/"},
	{"Tordex",					"http://www.tordex.com/"},
	{"Wiki: Web Browser",		"http://en.wikipedia.org/wiki/Web_browser?useskin=vector"},
	{"Wiki: Obama",			"http://en.wikipedia.org/wiki/Obama?useskin=vector"},
	{"std::vector",			"https://en.cppreference.com/w/cpp/container/vector"},

	{NULL,						NULL},
};

void CToolbarWnd::on_anchor_click( const char* url, const litehtml::element::ptr& el )
{
	if(!strcmp(url, "back"))
	{
		m_parent->back();
	} else if(!strcmp(url, "forward"))
	{
		m_parent->forward();
	} else if(!strcmp(url, "reload"))
	{
		m_parent->reload();
	} else if(!strcmp(url, "bookmarks"))
	{
		litehtml::position pos = el->get_placement();
		POINT pt;
		pt.x	= pos.right();
		pt.y	= pos.bottom();
		MapWindowPoints(m_hWnd, NULL, &pt, 1);

		HMENU hMenu = CreatePopupMenu();

		for(int i = 0; g_bookmarks[i].url; i++)
		{
			InsertMenu(hMenu, -1, MF_BYPOSITION | MF_STRING, i + 1, g_bookmarks[i].name);
		}

		int ret = TrackPopupMenu(hMenu, TPM_RIGHTALIGN | TPM_TOPALIGN | TPM_NONOTIFY | TPM_RETURNCMD, pt.x, pt.y, 0, m_hWnd, NULL);
		DestroyMenu(hMenu);

		if(ret)
		{
			m_parent->open(g_bookmarks[ret - 1].url);
		}
	} else if(!strcmp(url, "settings"))
	{
		litehtml::position pos = el->get_placement();
		POINT pt;
		pt.x	= pos.right();
		pt.y	= pos.bottom();
		MapWindowPoints(m_hWnd, NULL, &pt, 1);

		HMENU hMenu = CreatePopupMenu();

		InsertMenu(hMenu, -1, MF_BYPOSITION | MF_STRING, 1, "Calculate Render Time");
		InsertMenu(hMenu, -1, MF_BYPOSITION | MF_STRING, 3, "Calculate Render Time (10)");
		InsertMenu(hMenu, -1, MF_BYPOSITION | MF_STRING, 4, "Calculate Render Time (100)");
		InsertMenu(hMenu, -1, MF_BYPOSITION | MF_SEPARATOR, 0, "");
		InsertMenu(hMenu, -1, MF_BYPOSITION | MF_STRING, 5, "Calculate Draw Time");
		InsertMenu(hMenu, -1, MF_BYPOSITION | MF_STRING, 6, "Calculate Draw Time (10)");
		InsertMenu(hMenu, -1, MF_BYPOSITION | MF_STRING, 7, "Calculate Draw Time (100)");
		InsertMenu(hMenu, -1, MF_BYPOSITION | MF_SEPARATOR, 0, "");
		InsertMenu(hMenu, -1, MF_BYPOSITION | MF_STRING,	2, "Exit");

		int ret = TrackPopupMenu(hMenu, TPM_RIGHTALIGN | TPM_TOPALIGN | TPM_NONOTIFY | TPM_RETURNCMD, pt.x, pt.y, 0, m_hWnd, NULL);
		DestroyMenu(hMenu);

		switch(ret)
		{
		case 2:
			PostQuitMessage(0);
			break;
		case 1:
			m_parent->calc_time();
			break;
		case 3:
			m_parent->calc_time(10);
			break;
		case 4:
			m_parent->calc_time(100);
			break;
		case 5:
			m_parent->calc_redraw(1);
			break;
		case 6:
			m_parent->calc_redraw(10);
			break;
		case 7:
			m_parent->calc_redraw(100);
			break;
		}
	}
}

void CToolbarWnd::set_cursor( const char* cursor )
{
	m_cursor = cursor;
}

std::shared_ptr<litehtml::element> CToolbarWnd::create_element(const char* tag_name, const litehtml::string_map& attributes, const std::shared_ptr<litehtml::document>& doc)
{
	if (!litehtml::t_strcasecmp(tag_name, "input"))
	{
		auto iter = attributes.find("type");
		if (iter != attributes.end())
		{
			if (!litehtml::t_strcasecmp(iter->second.c_str(), "text"))
			{
				if (m_omnibox)
				{
					m_omnibox = nullptr;
				}

				m_omnibox = std::make_shared<el_omnibox>(doc, m_hWnd, this);
				return m_omnibox;
			}
		}
	}
	return 0;
}

void CToolbarWnd::load_image(const std::string& src, const std::string& baseurl, bool redraw_on_ready)
{
}

void CToolbarWnd::import_css(litehtml::string& text, const litehtml::string& url, litehtml::string& baseurl)
{

}

void CToolbarWnd::get_client_rect( litehtml::position& client ) const
{
	RECT rcClient;
	GetClientRect(m_hWnd, &rcClient);
	client.x		= rcClient.left;
	client.y		= rcClient.top;
	client.width	= rcClient.right - rcClient.left;
	client.height	= rcClient.bottom - rcClient.top;
}
