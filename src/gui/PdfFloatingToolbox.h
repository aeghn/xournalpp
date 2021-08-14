#include "FloatingToolbox.h"

#include "control/PDFTextSelectControl.h"
#include "GladeGui.h"
#include "control/Control.h"
#include "gui/XournalView.h"

#pragma once

class MainWindow;

class PdfFloatingToolbox {
public:
    PdfFloatingToolbox(MainWindow* theMainWindow, GtkOverlay* overlay);
    virtual ~PdfFloatingToolbox();

    PDFTextSelectControl* pdfTextSelectControl;

public:
    void show(int x, int y, PDFTextSelectControl* pdfTextSelectControl);
    void hide();

    void setSelectType(PdfTextSelectType type);
    PdfTextSelectType getSelectType();
    void switchSelectType();

private:
    void show();
    
    static gboolean getOverlayPosition(GtkOverlay* overlay, GtkWidget* widget, GdkRectangle* allocation,
                                       PdfFloatingToolbox* self);

    static void strikethroughCb(GtkButton* button, PdfFloatingToolbox* pft);
    static void underlineCb(GtkButton* button, PdfFloatingToolbox* pft);
    static void copyTextCb(GtkButton* button, PdfFloatingToolbox* pft);
    static void highlightCb(GtkButton* button, PdfFloatingToolbox* pft);
    static void closeCb(GtkButton* button, PdfFloatingToolbox* pft);
    static void switchSelectTypeCb(GtkButton* button, PdfFloatingToolbox* pft);
    
private:
    GtkWidget* floatingToolbox;

    PdfTextSelectType selectType;
    
    int floatingToolboxX = 0;
    int floatingToolboxY = 0;
};
