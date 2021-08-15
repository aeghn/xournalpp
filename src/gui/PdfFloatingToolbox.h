#pragma once

#include "control/tools/PdfTextSelection.h"
#include "gui/XournalView.h"
#include "control/Control.h"

#include "GladeGui.h"


enum PdfTextSelectType {
    SELECT_IN_AREA = 0,
    SELECT_HEAD_TAIL
};

class PdfFloatingToolbox {
public:
    PdfFloatingToolbox(MainWindow* theMainWindow, GtkOverlay* overlay);
    virtual ~PdfFloatingToolbox();

public:
    void show(int x, int y, PdfTextSelection* pdfTextSelection);
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

    void copyText();
    void createStrokes(PdfMarkerStyle position, PdfMarkerStyle width, int markerOpacity);
    void createStrokesForStrikethrough();
    void createStrokesForUnderline();
    void createStrokesForHighlight();
    void postAction();
    
private:
    GtkWidget* floatingToolbox = nullptr;
    MainWindow* theMainWindow = nullptr;

    PdfTextSelection* pdfTextSelection = nullptr;
    PdfTextSelectType selectType;
    
    int floatingToolboxX = 0;
    int floatingToolboxY = 0;
};
