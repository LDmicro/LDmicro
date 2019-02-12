/* (14-Nov-2016) [Tab/Indent: 8/8][Line/Box: 80/74]           (ScrollWnd.cpp) *
********************************************************************************
|                                                                              |
|                   Copyright (c) 2017, Robert C. Tausworthe                   |
|                             All Rights Reserved.                             |
|                         robert.c.tausworthe@ieee.org                         |
|                                                                              |
===============================================================================*

    Contents Description: Implementation of the CScrollWnd class for
    applications using the Win32++ Windows interface classes, Copyright
    (c) 2005-2017 David Nash, under permissions granted therein. Usage of
    this class is discussed below.

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

================================================================================

    Use of the ScrollWnd class:

    It is presumed that a CScrollWnd-derived view has been created having
    horizontal and/or vertical scroll bars by specifying the WM_HSCROLL
    and/or WM_HSCROLL options, and  that this view then handles showing
    and hiding of these bars, when appropriate, using he ShowXScrollBar()
    method, where X is either H or V. This view also sets the limits,
    increments, and  positioning of each bar, when visible.

    Each scroll bar position is assumed to range between certain minimum
    and maximum values, expressed in application-dependent (so-called
    logical) scroll units. At the maximum values, the positions of the
    scroll boxes, or "thumbs", are at the top and left scroll limits. At the
    minimum value, the thumbs are at the bottom and right scroll limits.
    Scrolling may be initiated by mouse clicks on the bars, by dragging the
    thumb boxes with the mouse, or by rotating the mouse wheel.

    Users may change the scrolling limits, increments, and position, and
    they may access the current values of limits, increments, and  position.
    It is up to the user then to draw the application client area in
    accordance with these limits, page sizes, increments, and  positions.

    It is common practice that the current scroll position refers to that
    part of the displayed material that appears at the top left corner of
    the view.

    Scroll units may be defined to fit the needs of the application. These
    units are termed "logical units". For example, if the view is displaying
    a screenful of lines of text read from a file, the convention may be to
    set the vertical limits to the first and last lines that can be
    displayed, and to set the horizontal limits to the first character and
    the length in characters of the longest line to be displayed. Similarly,
    the vertical increments may be to one text line line and a page size
    equal to the screen height. Horizontal increments may be 1 character and
    a page size equal to the screenwidth, in characters. In a case as this,
    the user is obligated to provide the mapping between logical scrolling
    and device units.

    If the view is a photograph or drawing, limits and increments may be
    scaled to represent pixels, or scaled to fit any of the CDC mapping
    modes, such as MM_TWIPS or MM_HIENGLISH. See the Microsoft MSDN
    documentation on CDC::SetMapMode for further details on such usage.

    In cases where the scroll parameters are scaled by a mapping mode,
    conversions that map the application's logical units to the device
    units--here, pixels--must be used. If the screen display is to be
    converted to another device, such as a printer context, then the device
    units change to fit that context, such as dots in the printer context.

    It is therefore necessary in such cases, to maintain device
    independence, to choose a mapping mode other than MM_TEXT, which
    equates logical units to device units, i.e., pixels or dots. The choice
    of this mapping mode is up to the application. When the derived view
    class needs to translate the scroll position for display purposes,
    the device context function LPtoDP() can be used to convert logical
    units to device units and its inverse, DPtoLP() can be used to convert
    device units to logical units. The device context class knows how to
    make the appropriate transformations for the selected device in these
    mapping modes.

    Design Considerations: This class is built using all of the CWnd base
    class scrolling functions, except for EnableScrollBar, ScrollWindow, and
    ScrollWindowEx. The latter two were deemed unnecessary due to the double
    buffering provided in the CScrollWnd::OnDraw() member. No use is made of
    the CScrollBar class.

    The use of scrolling functions is explained in the article,

        http://msdn.microsoft.com/en-us/library/ms997557.aspx

    However, much of the user interactions with WINAPI functions in the
    article has been subsumed within CScrollWnd member methods.

    Several member methods are expected to be overriden in classes derived
    from CScrollWnd. These are

        SetNewAppSize
        Paint
        SetScrollAppInfo

    Others may be optionally overridden, identified by their virtual
    declaration in CScrollWnd.h

================================================================================

    Programming Notes:
                The programming standards roughly follow those established
                by the 1997-1999 Jet Propulsion Laboratory Network Planning
        and Preparation Subsystem project for C++ programming.

*******************************************************************************/

#include "stdafx.h"
#include "ScrollWnd.hpp"
#include "resource.h"


static const COLORREF rgbDefaultBkColor       = RGB(255, 255, 255);
static const UINT current_ScrollRanges_schema     = 1;
static const UINT current_ScrollIncrements_schema = 1;
static const UINT current_ScrollWin_schema        = 1;

/*******************************************************************************

    Implementation of CScrollWnd class

*=============================================================================*/
    CScrollWnd::
CScrollWnd()                                            /*

    Construct the scroll window object.
*-----------------------------------------------------------------------------*/
{
    m_nMapMode   = MM_TEXT;  // default coordinate mapping mode
    m_rgbBkColor = rgbDefaultBkColor;
    m_brBkGnd    = CBrush(m_rgbBkColor);
    m_bIamBusy   = FALSE;
    SetScrollPosition(CPoint(0, 0));
}

/*============================================================================*/
    CPoint CScrollWnd::
DevToScrl(CPoint pt) const                          /*

    Return the device image of the logical position pt. The base class
    uses the current mapping mode, presumed to be one of the valid CDC
    mapping modes.
*-----------------------------------------------------------------------------*/
{
    assert(MM_MIN <= m_nMapMode && m_nMapMode <= MM_MAX);
    CClientDC dc(*this);
    dc.SetMapMode(m_nMapMode);
    dc.LPtoDP(&pt, 1);
    return pt;
}

/*============================================================================*/
    CPoint CScrollWnd::
GetScrollPosition() const                       /*

    Return the current scroll bar position coordinates.
*-----------------------------------------------------------------------------*/
{
    if (!IsWindow())
        return CPoint(0, 0);

    return CPoint(GetScrollPos(SB_HORZ), GetScrollPos(SB_VERT));
}

/*============================================================================*/
    BOOL CScrollWnd::
IsHVisible() const                              /*

    Return TRUE if the horizontal scroll bar is visible, FALSE otherwise.
*-----------------------------------------------------------------------------*/
{
    return (GetStyle() &  WS_HSCROLL) != 0;
}

/*============================================================================*/
    BOOL CScrollWnd::
IsVVisible() const                              /*

    Return TRUE if the vertical scroll bar is visible, FALSE otherwise.
*-----------------------------------------------------------------------------*/
{
    return (GetStyle() &  WS_VSCROLL) != 0;
}

/*============================================================================*/
    void CScrollWnd::
OnDraw(CDC& sDC)                                                        /*

    Draw and display the application scroll window's owner-supplied content.
    Called when part or all of the window needs to be redrawn. There is no
    need to override this method unless buffered display of the window is
    unwanted.
*-----------------------------------------------------------------------------*/
{
      // The drawing of the client area is done in a manner that reduces
      // flicker when the window is scrolled or resized. This is done by
      // creating a compatible DC and bitmap, painting what is to appear
      // in the window into this bitmap, and, once complete, copying this
      // bitmap into the window.  This process is split into two parts: The
      // part that comprises this method is application-independent. The
      // remainder is relegated to the Paint() method. The WindProc()
      // function further responds to the WM_ERASEBKGND by not erasing the
      // background. We start by creating the device context for handling
      // the WM_PAINT message.
    CRect rc = GetClientRect(); // device units (pixels)
    // create a memory device context to print into
    CDC dcMem;
    dcMem.CreateCompatibleDC(sDC);
      // Create a bitmap big enough for our client rectangle.
    dcMem.CreateCompatibleBitmap(sDC, rc.Width(), rc.Height());
    dcMem.FillRect(rc, m_brBkGnd);
      // here we would paint the client rc with app-dependent stuff
    Paint(dcMem);
      // now we can rapidly copy the painted memory into the screen DC.
    sDC.BitBlt(rc.left, rc.top, rc.Width(), rc.Height(),
        dcMem, 0, 0, SRCCOPY);
}

/*============================================================================*/
        LRESULT CScrollWnd::
OnHScroll(WPARAM wParam, LPARAM lParam)                     /*

        Processes horizontal scrolling requests. The scroll code is given in
    LOWORD(wParam).  HIWORD(wParam) specifies the current position of
    the scroll box if the scroll code is SB_THUMBPOSITION or SB_THUMBTRACK;
    but here it is not used. The lParam is the handle to the scroll bar
    control if the message was sent by the scroll bar; otherwise, it is
    NULL. Thererfore, since we have messages coming from the keyboard
    as well as the scrollbar, only the scroll code is used here. The
    function returns 0.
*-----------------------------------------------------------------------------*/
{
    UNREFERENCED_PARAMETER(lParam);
    if (!IsHVisible())
        return 0;

    if (!IsWindow())
        return 0;

      // respond to the scroll event:
    SCROLLINFO si;
    si.cbSize = sizeof(si);
    si.fMask  = SIF_ALL;
    GetScrollInfo(SB_HORZ, si);
    CPoint pos = GetScrollPosition();
      // need horizontal scroll bar position and thumb track position
    switch (LOWORD(wParam)) // scroll command
    {
        case SB_LEFT:       //   Scroll to far left.
        pos.x = si.nMin;
        break;

        case SB_RIGHT:      //   Scroll to far right.
        pos.x = si.nMax;
        break;

        case SB_LINELEFT:           // scroll left one column-unit
        pos.x -= m_line_increment.cx;
        break;

        case SB_LINERIGHT:           // scroll right one column-unit
        pos.x += m_line_increment.cx;
        break;

        case SB_PAGELEFT:           // scroll left one column-page-unit
        pos.x -= si.nPage;
        break;

        case SB_PAGERIGHT:           // scroll right one column-page-unit
        pos.x += si.nPage;
        break;

        case SB_THUMBTRACK:     // h-thumb is moving
        case SB_THUMBPOSITION:      // or has stopped
        pos.x = si.nTrackPos;   // was HIWORD(wParam);
        break;

        default:    // the rest are immaterial
        break;
    }
      // Reset new horizontal scroll position
    SetScrollPosition(pos);
    Invalidate(FALSE);
    return 0;
}

/*============================================================================*/
    void CScrollWnd::
OnInitialUpdate()                                                       /*

    Called immediately after the window is created. Override this method to
    prepare the initial application window.
*-----------------------------------------------------------------------------*/
{
      // set initial limits
    SetScrollRanges(0, 100, 0, 100);
      // set initial increments
    SetScrollIncrements(1, 1, 1, 1);
      // hide the scroll bars until needed
    TRACE("CScrollWnd window created\n");
}

/*============================================================================*/
        LRESULT CScrollWnd::
OnKeyScroll(WPARAM wParam, LPARAM lParam)                   /*

    Processes nonsystem (i.e., non ALT+key) keystrokes: wParam contains
    the virtual key code. lParam combines repeat count, OEM scan code,
    extended-key flag, and  previous key-state flag.  Only the virtual
    key code is used here.

    The usual scrolling functions of keys are implemented here.
*-----------------------------------------------------------------------------*/
{
    UNREFERENCED_PARAMETER(lParam);

    if (!IsWindow())
        return 0;

    UINT    wScrollNotify = 0,
        uMessage = 0;

      // get the state of the control key
    BOOL control = ((::GetKeyState(VK_CONTROL) & 0x8000) ? TRUE : FALSE);
      // respond to the scroll event:
    switch (wParam)
    {
            case VK_HOME:       // HOME key
        if (control)
        {         // top limit of scroll
                wScrollNotify = SB_TOP;
                uMessage = WM_VSCROLL;
                SendMessage(uMessage,
                MAKELONG(wScrollNotify, 0), 0L);
                wScrollNotify = SB_LEFT;
                uMessage = WM_HSCROLL;
        }
        else
        {         // left limit of scroll
                wScrollNotify = SB_LEFT;
                uMessage = WM_HSCROLL;
        }
            break;

        case VK_END:        // END key
        if (control)
        {         // bottom limit of scroll
                wScrollNotify = SB_BOTTOM;
                uMessage = WM_VSCROLL;
                SendMessage(uMessage,
                MAKELONG(wScrollNotify, 0), 0L);
                wScrollNotify = SB_LEFT;
                uMessage = WM_HSCROLL;
        }
        else
        {         // right limit of scroll
                wScrollNotify = SB_RIGHT;
                uMessage = WM_HSCROLL;
        }
            break;

        case VK_PRIOR:      //PAGEUP key
            wScrollNotify = SB_PAGEUP;
            uMessage = WM_VSCROLL;
            break;

        case VK_NEXT:       // PAGEDOWN key
            wScrollNotify = SB_PAGEDOWN;
            uMessage = WM_VSCROLL;
            break;

        case VK_UP:         // UPARROW key
        if (control)
        {         // one vertical increment up
            wScrollNotify = SB_LINEUP;
                uMessage = WM_VSCROLL;
            break;
        }

        case VK_LEFT:       // LEFTARROW key
        if (control)
        {         // one column increment left
                wScrollNotify = SB_LINELEFT;
                uMessage = WM_HSCROLL;
            break;
        }

        case VK_RIGHT:      // RIGHTARROW key
        if (control)
        {         // one column increment right
                wScrollNotify = SB_LINERIGHT;
                uMessage = WM_HSCROLL;
            break;
        }

        case VK_DOWN:       // DOWNARROW key
        if (control)
        {         // one vertical increment down
                wScrollNotify = SB_LINEDOWN;
                uMessage = WM_VSCROLL;
            }
            break;

        default:
            wScrollNotify = 0xFFFF;
            break;
    }
      // send the keystroke message
    if (wScrollNotify != 0xFFFF)
        SendMessage(uMessage, MAKELONG(wScrollNotify, 0), 0L);

    return 0;
}

/*============================================================================*/
        LRESULT CScrollWnd::
OnMouseWheel(WPARAM wParam, LPARAM lParam)                  /*

        Processes mouse wheel rotation when made in the focus window having
    a vertical scrollbar.  Although LOWORD(wParam) contains the key flags,
    HIWORD(wParam) gives the (signed) wheel rotation, GET_X_LPARAM(lParam)
    is the horizontal position of the pointer, and  GET_Y_LPARAM(lParam)
    is the vertical position of pointer, theseflags and  positions are
    unused here.  The incoming rotation parameter will be a multiple of
    the WHEEL_DELTA value, which is set at 120. Each such unit here scrolls
    one virtical increment. The function returns 0.
*-----------------------------------------------------------------------------*/
{
    UNREFERENCED_PARAMETER(lParam);

    if (!IsVVisible())
        return 0;

      // set the new position: first get (signed) rotation units
    int wheelunits = HIWORD(wParam),
      // convert to line increments, maintaining sign
    rotation = ::MulDiv(-wheelunits, m_line_increment.cy, WHEEL_DELTA);
    CPoint pos = GetScrollPosition();
    pos.y += rotation;
    SetScrollPosition(pos);
    Invalidate(FALSE);
    return 0;
}

/*============================================================================*/
        LRESULT CScrollWnd::
OnVScroll(WPARAM wParam, LPARAM lParam)                     /*

        This function processes vertical scrolling requests. The scroll code
    is given in LOWORD(wParam). HIWORD(wParam) specifies the current
    scroll box if the scroll code is SB_THUMBPOSITION or SB_THUMBTRACK;
    but here it is not used. lParam is the handle to the scroll bar
    control if the message was sent by the scroll bar; otherwise,
    it is NULL. Thererfore, since we have messages coming from the
    keyboard as well as the scrollbar, only the scroll code is used here.
    The function returns 0.
*-----------------------------------------------------------------------------*/
{
    UNREFERENCED_PARAMETER(lParam);

    if (!IsVVisible())
        return 0;

      // respond to the scroll event:
    SCROLLINFO si;
    si.cbSize = sizeof(si);
    si.fMask  = SIF_ALL;
    GetScrollInfo(SB_VERT, si);
    CPoint pos = GetScrollPosition();
    switch (LOWORD(wParam))  // the scroll code
    {
        case SB_TOP:                // top of scroll
        pos.y = si.nMin;
        break;

        case SB_BOTTOM:             // bottom of scroll
        pos.y = si.nMax;
        break;

        case SB_LINEUP:             // up one line-unit
        pos.y -= m_line_increment.cy;
        break;

        case SB_LINEDOWN:           // down one line-unit
        pos.y += m_line_increment.cy;
        break;

        case SB_PAGEUP:             // up one page-unit
        pos.y -= si.nPage;
        break;

        case SB_PAGEDOWN:           // down one page-unit
        pos.y += si.nPage;
        break;

        case SB_THUMBTRACK:         // the thumb is moving
        case SB_THUMBPOSITION:      // or has stopped
          // need THE thumb track position
        pos.y = si.nTrackPos; // was HIWORD(wParam);
        break;

        default:
        break;
    }
      // Reset new vertical scroll position
    SetScrollPosition(pos);
    Invalidate(FALSE);
    return 0;
}

/*============================================================================*/
        void CScrollWnd::
Paint(CDC& dcMem)                                               /*

    Paint the window's compatible bitmap whose device context is dcMem
    bounded by the rectangle rc, in pixel units. The base class does
    nothing. Override this member to display the application window.
*-----------------------------------------------------------------------------*/
{
    UNREFERENCED_PARAMETER(dcMem);
}

/*============================================================================*/
    void CScrollWnd::
PreCreate(CREATESTRUCT &cs)                                             /*

    Set the defaults to be used by the create function to identify this as
    a child scrolling window.  Override this method to add any specific
    attributes for the using application.
*-----------------------------------------------------------------------------*/
{
      // a child window of the main frame, with h- and v-scrolling
    cs.style = WS_CHILD | WS_HSCROLL | WS_VSCROLL;
}

/*============================================================================*/
    void CScrollWnd::
PreRegisterClass(WNDCLASS &wc)                                          /*

    Identify the Window class as a scrolling window with the default color
    background.
*-----------------------------------------------------------------------------*/
{
      // Set the Window Class name
    wc.lpszClassName = _T("CScrollWnd");
      // Set the background color to the default
    wc.hbrBackground = (HBRUSH)m_brBkGnd;
    wc.hCursor = ::LoadCursor(NULL, IDC_ARROW);
}

/*============================================================================*/
    BOOL CScrollWnd::
PreTranslateMessage(MSG &msg)                                           /*

    Used by CWinApp to translate window messages before they are dispatched
    to theTranslateMessage and DispatchMessage Windows functions in the
    message loop. MSG contains the message to process. Return a nonzero
    if the message was translated and should not be dispatched; return
    0 if the message was not translated and should be dispatched.
*-----------------------------------------------------------------------------*/
{
    UNREFERENCED_PARAMETER(msg);

//  HWND   hwnd = msg->hwnd;
//  UINT   message  = msg->message;
//  WPARAM wParam   = msg->wParam;
//  LPARAM lParam   = msg->lParam;
//  DWORD  time = msg->time;
//  CPoint  pt  = msg->pt;

      // return 0 if the message was NOT handled here
    return 0;
}

/*============================================================================*/
    CPoint CScrollWnd::
ScrlToDev(CPoint pt) const                      /*

    Return the logical position corresponding to the device postion pt.
    The mapping mode is assumed to be one of the valid CDC mappings.
*-----------------------------------------------------------------------------*/
{
    assert(MM_MIN <= m_nMapMode && m_nMapMode <= MM_MAX);
    CClientDC dc(*this);
    dc.SetMapMode(m_nMapMode);
    dc.DPtoLP(&pt, 1);
    return pt;
}

/*============================================================================*/
        void CScrollWnd::
Serialize(CArchive &ar)                                               /*

        Serialize or deserialize the scroll window to and from the archive ar,
    depending on the sense of IsStoring(). This class has its own m_nSchema.
    (The saved values are not used in the present application.)
*-----------------------------------------------------------------------------*/
{
      // perform loading or storing
        if (ar.IsStoring())
        {
                  // save background color
        ar << m_rgbBkColor;
    }
        else
        {
          // recover background color
                ar >> m_rgbBkColor;
        m_brBkGnd = CBrush(m_rgbBkColor);
        }
}

/*============================================================================*/
    void CScrollWnd::
SetNewViewSize()                            /*

    Compute the logical view size of the client window that may or may not
    have scroll bars present. Each scroll bar will be assumed to appear when
    the extent of the application either exceeds the size of the client
    size when no scroll bar blocks its edge, or exceeds the adjusted
    client size when a scroll bar is present. On exit, the determined sizes
    may be used directly to determine the presence of scroll bars. The
    method for determining scroll bar presence is derived in the article,
    ScrollBarsCriteria.pdf, and the notation used here is that of the
    article.
*-----------------------------------------------------------------------------*/
{
      // the overall client size, without scroll bars
    CRect rc = GetClientRect();
    CSize client(rc.Width(), rc.Height()),
          view = client;
    if (m_app_size.cx != 0 && m_app_size.cy != 0)
    {
        CSize app = ScrlToDev(m_app_size);
          // get the scroll bar sizes
        int scrollbar_x = ::GetSystemMetrics(SM_CXVSCROLL),
            scrollbar_y = ::GetSystemMetrics(SM_CYHSCROLL);
        BOOL A = (app.cx > client.cx - scrollbar_x),
             B = app.cx > client.cx,
             C = (app.cy > client.cy - scrollbar_y),
             D = app.cy > client.cy;
        if (B || (A && D))
            view.cx -= scrollbar_x;
        if (D || (B && C))
            view.cy -= scrollbar_y;
    }
    m_client_size = DevToScrl(client);
    m_view_size   = DevToScrl(view);
}

/*============================================================================*/
    void CScrollWnd::
SetScrollIncrements(int hLine, int hPage, int vLine, int vPage)     /*

    Set the current scrolling increments to the argument values. The page
    increments are also entered into the SCROLLINFO data for each bar.
*-----------------------------------------------------------------------------*/
{
    if (!IsWindow())
        return;

    m_line_increment.cx = hLine;
    m_line_increment.cy = vLine;
    SCROLLINFO si;
    si.cbSize = sizeof (si);
    si.fMask = SIF_PAGE;
    si.nPage = hPage;
    SetScrollInfo(SB_HORZ, si, TRUE);
    si.nPage = vPage;
    SetScrollInfo(SB_VERT, si, TRUE);
}

/*============================================================================*/
    void    CScrollWnd::
SetScrollParameters()                                           /*

    Compute the scroll ranges and increments for the current app and view.
*-----------------------------------------------------------------------------*/
{
    if (m_app_size.cy == 0)
        return;

      // prevent recursive invocations
    if (m_bIamBusy)
        return;

    m_bIamBusy = TRUE;
     // get the new size
    SetNewViewSize();
    SetScrollAppInfo(); // ranges and increments
      // show/hide the scrollbars
    ShowHScrollBar(m_app_size.cx > m_view_size.cx);
    ShowVScrollBar(m_app_size.cy > m_view_size.cy);
    CPoint sp = GetScrollPosition();
      // if horizontal bar is hidden, render flush left
    if (!IsHVisible())
        sp.x = 0;
      // if vertical bar is hidden, flush top
    if (!IsVVisible())
        sp.y = 0;
    SetScrollPosition(sp);
    Invalidate();
      // ok for next time
    m_bIamBusy = FALSE;
}

/*============================================================================*/
    void CScrollWnd::
SetScrollPosition(CPoint pos)                       /*

    Set the horizontal and  vertical scroll positions, respectively, to
    pos.x and pos.y, respectively and redraw the scroll bars. Limit values
    that go out of bounds.
*-----------------------------------------------------------------------------*/
{
    if (!IsWindow())
        return;

    SetScrollPos(SB_HORZ, pos.x, TRUE);
    SetScrollPos(SB_VERT, pos.y, TRUE);
}

/*============================================================================*/
    void CScrollWnd::
SetScrollRanges(int hMin, int hMax, int vMin, int vMax)         /*

    Set the current scrolling limits to parameter values.
*-----------------------------------------------------------------------------*/
{
    if (!IsWindow())
        return;

    SCROLLINFO si;
    si.cbSize = sizeof (si);
    si.fMask = SIF_RANGE;
    si.nMax  = hMax;
    si.nMin  = hMin;
    SetScrollInfo(SB_HORZ, si, TRUE);
    si.nMax  = vMax;
    si.nMin  = vMin;
    SetScrollInfo(SB_VERT, si, TRUE);
}

/*============================================================================*/
        void CScrollWnd::
ShowHScrollBar(BOOL show) const                                         /*

    Show or hide the horizontal scroll bar, as per the show parameter.
*-----------------------------------------------------------------------------*/
{
    if (!IsWindow())
        return;

    ShowScrollBar(SB_HORZ, show);
}

/*============================================================================*/
        void CScrollWnd::
ShowVScrollBar(BOOL show) const                                         /*

    Show or hide the vertical scroll bar, as per the show parameter.
*-----------------------------------------------------------------------------*/
{
    if (!IsWindow())
        return;

    ShowScrollBar(SB_VERT, show);
}

/*============================================================================*/
    LRESULT CScrollWnd::
WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam)                        /*

    All messages for this window pass through this WndProc.  In particular,
    handling scrollbar messages are dispatched here.
*-----------------------------------------------------------------------------*/
{
    switch (uMsg)
    {
        case WM_SIZE:
        SetScrollParameters();
        break;  // Also do the default processing

        case WM_ERASEBKGND:
        return 1;       // prevents flicker on resizing

        case WM_VSCROLL:    // received when the v-scrollbar is clicked
                if (OnVScroll(wParam, lParam) == 0)
                        return 0;

                break;

        case WM_HSCROLL:    // received when the v-scrollbar is clicked
                if (OnHScroll(wParam, lParam) == 0)
                        return 0;

                break;

        case WM_KEYDOWN:  // received when a key is pressed
                if (OnKeyScroll(wParam, lParam) == 0)
                        return 0;

        break;

        case WM_MOUSEWHEEL:  // rotation detected
        if (OnMouseWheel(wParam, lParam) == 0)
            return 0;

        break;
    }
      // pass unhandled messages on for default processing
    return WndProcDefault(uMsg, wParam, lParam);
}
