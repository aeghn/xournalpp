#include "control/PDFTextSelectControl.h"

#include "FloatingToolbox.h"
#include "GladeGui.h"

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

private:
    static gboolean getOverlayPosition(GtkOverlay* overlay, GtkWidget* widget, GdkRectangle* allocation,
                                       PdfFloatingToolbox* self);
    void show();

    static void strikethroughCb(GtkButton* button, PdfFloatingToolbox* pft);
    static void underlineCb(GtkButton* button, PdfFloatingToolbox* pft);
    static void copyTextCb(GtkButton* button, PdfFloatingToolbox* pft);
    static void highlightCb(GtkButton* button, PdfFloatingToolbox* pft);
    static void closeCb(GtkButton* button, PdfFloatingToolbox* pft);

private:
    MainWindow* mainWindow;
    GtkWidget* floatingToolbox;

    int floatingToolboxX = 0;
    int floatingToolboxY = 0;
};