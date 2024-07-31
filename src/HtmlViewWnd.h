#pragma once
#include "web_page.h"
#include "web_history.h"

#define HTMLVIEWWND_CLASS	"HTMLVIEW_WINDOW"

#define WM_IMAGE_LOADED		(WM_USER + 1000)
#define WM_PAGE_LOADED		(WM_USER + 1001)

using namespace litehtml;
class CBrowserWnd;

class CHTMLViewWnd
{
	HWND						m_hWnd;
	HINSTANCE					m_hInst;
	int							m_top;
	int							m_left;
	int							m_max_top;
	int							m_max_left;
	web_history					m_history;
	web_page*					m_page;
	web_page*					m_page_next;
	CRITICAL_SECTION			m_sync;
	simpledib::dib				m_dib;
	CBrowserWnd*				m_parent;
public:
	CHTMLViewWnd(HINSTANCE	hInst, CBrowserWnd* parent);
	virtual ~CHTMLViewWnd(void);

	void				create(int x, int y, int width, int height, HWND parent);
	void				open(LPCSTR url, bool reload = FALSE);
	HWND				wnd()	{ return m_hWnd;	}
	void				refresh();
	void				back();
	void				forward();

	void				set_caption();
	void				lock();
	void				unlock();
	bool				is_valid_page(bool with_lock = true);
	web_page*			get_page(bool with_lock = true);

	void				render(BOOL calc_time = FALSE, BOOL do_redraw = TRUE, int calc_repeat = 1);
	void				get_client_rect(litehtml::position& client) const;
	void				show_hash(std::string& hash);
	void				update_history();
	void				calc_draw(int calc_repeat = 1);

protected:
	virtual void		OnCreate();
	virtual void		OnPaint(simpledib::dib* dib, LPRECT rcDraw);
	virtual void		OnSize(int width, int height);
	virtual void		OnDestroy();
	virtual void		OnVScroll(int pos, int flags);
	virtual void		OnHScroll(int pos, int flags);
	virtual void		OnMouseWheel(int delta);
	virtual void		OnKeyDown(UINT vKey);
	virtual void		OnMouseMove(int x, int y);
	virtual void		OnLButtonDown(int x, int y);
	virtual void		OnLButtonUp(int x, int y);
	virtual void		OnMouseLeave();
	virtual void		OnPageReady();
	
	void				redraw(LPRECT rcDraw, BOOL update);
	void				update_scroll();
	void				update_cursor();
	void				create_dib(int width, int height);
	void				scroll_to(int new_left, int new_top);
	

private:
	static LRESULT	CALLBACK WndProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);
};

inline void CHTMLViewWnd::lock()
{
	EnterCriticalSection(&m_sync);
}

inline void CHTMLViewWnd::unlock()
{
	LeaveCriticalSection(&m_sync);
}
