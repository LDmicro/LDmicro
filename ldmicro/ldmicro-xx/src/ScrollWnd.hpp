/* (02-Aug-2016) [Tab/Indent: 8/8][Line/Box: 80/74]             (ScrollWnd.h) *
********************************************************************************
|                                                                              |
|                   Copyright (c) 2017, Robert C. Tausworthe                   |
|                             All Rights Reserved.                             |
|                         robert.c.tausworthe@ieee.org                         |
|                                                                              |
===============================================================================*

    Contents Description: Declaration of the CScrollWnd class for
    applications using the Win32++ Windows interface classes, Copyright
    (c) 2005-2017 David Nash, under permissions granted therein.

        Caveats: The copyright displayed above extends only to the author's
    original contributions to the subject class, and to the alterations,
    additions, deletions, and other treatments of materials that may have
    been extracted from the cited sources.  Unaltered portions of those
    materials retain their original copyright status. The author hereby
    grants permission to any person obtaining a copy of this treatment
    of the subject class and any associated documentation composed by
    the author, to utilize this material, free of charge and without
    restriction or limitation, subject to the following conditions:

        The above copyright notice, as well as that of David Nash
        and Win32++, together with the respective permission
        conditions shall be included in all copies or substantial
        portions of this material so copied, modified, merged,
        published, distributed, or otherwise held by others.

    These materials are provided "as is", without warranty of any kind,
    express or implied, including but not limited to: warranties of
    merchantability, fitness for a particular purpose, and non-infringement.
    In no event shall the authors or copyright holders be liable for any
    claim, damages, or other liability, whether in an action of contract,
    tort or otherwise, arising from, out of, or in connection with, these
    materials, the use thereof, or any other other dealings therewith.

    Special Conventions:

    Programming Notes:
                The programming standards roughly follow those established
                by the 1997-1999 Jet Propulsion Laboratory Network Planning
        and Preparation Subsystem project for C++ programming.

*******************************************************************************/

#ifndef CSCROLLWIN_H_DEFINED
#define CSCROLLWIN_H_DEFINED

#include "precomp/ldmicroxxprecomphdr.hpp"

#ifndef WHEEL_DELTA
#define WHEEL_DELTA 120
#endif // WHEEL_DELTA

#ifndef MM_MIN
#define MM_MIN MM_TEXT
#endif // MM_MIN

#ifndef MM_MAX
#define MM_MAX MM_ANISOTROPIC
#endif // MM_MAX

const SIZE ScrollSizeDefault = {0, 0};

/*============================================================================*/
class CScrollWnd : public CWnd /*

    A scrollable window class.
*-----------------------------------------------------------------------------*/
{
public:
    CScrollWnd();
    virtual ~CScrollWnd() {}

    // public method members
    virtual CPoint DevToScrl(CPoint) const;
    CPoint         GetScrollPosition() const;
    virtual CPoint ScrlToDev(CPoint) const;
    virtual void   SetNewAppSize() { m_app_size = CSize(0, 0); }
    virtual void   SetNewViewSize();
    void           SetScrollPosition(CPoint pt);

protected:
    CSize        GetAppSize() const { return m_app_size; };
    CSize        GetClientSize() const { return m_client_size; };
    int          GetMappingMode() const { return m_nMapMode; }
    CSize        GetViewSize() { return m_view_size; };
    COLORREF     GetWndBkColor() { return m_rgbBkColor; }
    BOOL         IsHVisible() const;
    BOOL         IsVVisible() const;
    virtual void OnDraw(CDC& sDC);
    LRESULT      OnHScroll(WPARAM, LPARAM);
    virtual void OnInitialUpdate();
    LRESULT      OnKeyScroll(WPARAM, LPARAM);
    LRESULT      OnMouseWheel(WPARAM, LPARAM);
    LRESULT      OnVScroll(WPARAM, LPARAM);
    virtual void Paint(CDC& dcMem);
    virtual void PreCreate(CREATESTRUCT& cs);
    virtual void PreRegisterClass(WNDCLASS& wc);
    virtual BOOL PreTranslateMessage(MSG& Msg);
    virtual void Serialize(CArchive& ar);
    void         SetAppSize(CSize sz) { m_app_size = sz; }
    void         SetMappingMode(int mapMode)
    {
        m_nMapMode = mapMode;
    }
    void         SetScrollIncrements(int, int, int, int);
    virtual void SetScrollAppInfo() {}
    void         SetScrollParameters();
    void         SetScrollRanges(int, int, int, int);
    void         SetViewSize(CSize sz) { m_view_size = sz; }
    void         SetWndBkColor(COLORREF rgb)
    {
        m_rgbBkColor = rgb;
        m_brBkGnd    = CBrush(rgb);
    }
    void            ShowHScrollBar(BOOL show) const;
    void            ShowVScrollBar(BOOL show) const;
    virtual LRESULT WndProc(UINT uMsg, WPARAM, LPARAM);

private:
    // private data members
    int   m_nMapMode;       // point mapping mode
    CSize m_line_increment, // per line scroll, logical
        m_app_size,         // app size, logical
        m_client_size,      // client size, logical
        m_view_size;        // adjusted size, logical
    COLORREF m_rgbBkColor;  // window background color
    CBrush   m_brBkGnd;     // window background brush
    BOOL     m_bIamBusy;    // prevent recursive msgs
};
/*----------------------------------------------------------------------------*/
#endif // CSCROLLWIN_H_DEFINED
