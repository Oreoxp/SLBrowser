#pragma once

#define BROWSERWND_CLASS	"BROWSER_WINDOW"

class CHTMLViewWnd;
class CToolbarWnd;

class CBrowserWnd
{
	HWND				m_hWnd;
	HINSTANCE			m_hInst;
	CHTMLViewWnd*		m_view;
#ifndef NO_TOOLBAR
	CToolbarWnd*		m_toolbar;
#endif
public:
	CBrowserWnd(HINSTANCE hInst);
	virtual ~CBrowserWnd(void);

	void create();
	void open(LPCSTR path);

	void back();
	void forward();
	void reload();
	void calc_time(int calc_repeat = 1);
	void calc_redraw(int calc_repeat = 1);
	void on_page_loaded(LPCSTR url);

protected:
	virtual void OnCreate();
	virtual void OnSize(int width, int height);
	virtual void OnDestroy();

private:
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);
};
