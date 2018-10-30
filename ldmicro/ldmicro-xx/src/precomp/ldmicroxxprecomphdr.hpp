/*******************************************************************************
*  file    : ldmicroxxprecomphdr.hpp
*  created : 15.08.2018
*  author  : Slyshyk Oleksiy (alexslyshyk@gmail.com)
*******************************************************************************/
#ifndef LDMICROXXPRECOMPHDR_HPP
#define LDMICROXXPRECOMPHDR_HPP

// Rarely modified header files should be included here
#include <vector>               // Add support for std::vector
#include <map>                  // Add support for std::map
#include <string>               // Add support for std::string
#include <sstream>              // Add support for stringstream
#include <assert.h>             // Add support for the assert macro
#include <stdio.h>              // Add support for C style printf, sprintf, etc.
#include <stdlib.h>             // Add support for C style character conversions atoi etc.
#include <tchar.h>              // Add support for C style TCHARs.

#include <wxx_appcore.h>        // Add CCriticalSection, CObject, CWinThread, CWinApp
#include <wxx_archive.h>        // Add CArchive
#include <wxx_commondlg.h>      // Add CCommonDialog, CColorDialog, CFileDialog, CFindReplace, CFontDialog
#include <wxx_controls.h>       // Add CAnimation, CComboBox, CComboBoxEx, CDateTime, CHeader, CHotKey, CIPAddress, CProgressBar, CSpinButton, CScrollBar, CSlider, CToolTip
#include <wxx_cstring.h>        // Add CString, CStringA, CStringW
#include <wxx_ddx.h>            // Add CDataExchange
#include <wxx_dialog.h>         // Add CDialog, CResizer
#include <wxx_dockframe.h>      // Add CDockFrame, CMDIDockFrame
#include <wxx_docking.h>        // Add CDocker, CDockContainer
#include <wxx_exception.h>      // Add CException, CFileException, CNotSupportedException, CResourceException, CUserException, CWinException
#include <wxx_file.h>           // Add CFile
#include <wxx_filefind.h>       // Add CFileFind
#include <wxx_folderdialog.h>   // Add CFolderDialog
#include <wxx_frame.h>          // Add CFrame
#include <wxx_gdi.h>            // Add CDC, CGDIObject, CBitmap, CBrush, CFont, CPalatte, CPen, CRgn
#include <wxx_imagelist.h>      // Add CImageList
#include <wxx_listview.h>       // Add CListView
#include <wxx_mdi.h>            // Add CMDIChild, CMDIFrame, CDockMDIFrame
#include <wxx_metafile.h>       // Add CMetaFile, CEnhMetaFile
#include <wxx_printdialogs.h>   // Add CPageSetupDialog, CPrintSetupDialog
#include <wxx_propertysheet.h>  // Add CPropertyPage, CPropertySheet
#include <wxx_rebar.h>          // Add CRebar
#include <wxx_regkey.h>         // Add CRegKey
//#include <wxx_ribbon.h>       // Add CRibbon, CRibbonFrame
#include <wxx_richedit.h>       // Add CRichEdit
#include <wxx_scrollview.h>     // Add CScrollView
#include <wxx_shared_ptr.h>     // Add Shared_Ptr
//#include <wxx_socket.h>         // Add CSocket
#include <wxx_statusbar.h>      // Add CStatusBar
#include <wxx_stdcontrols.h>    // Add CButton, CEdit, CListBox
#include <wxx_tab.h>            // Add CTab, CTabbedMDI
//#include <wxx_taskdialog.h>   // Add CTaskDialog
#include <wxx_time.h>           // Add CTime
#include <wxx_toolbar.h>        // Add CToolBar
#include <wxx_treeview.h>       // Add CTreeView
#include <wxx_webbrowser.h>     // Add CAXWindow, CWebBrowser
#include <wxx_wincore.h>        // Add CWnd

#endif // LDMICROXXPRECOMPHDR_HPP
