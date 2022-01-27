#pragma once

#include "control/tools/PdfElemSelection.h"
#include "gui/XournalView.h"

#include "GladeGui.h"

class MainWindow;

enum class PdfMarkerStyle : uint8_t {
    POS_TEXT_BOTTOM = 0,
    POS_TEXT_MIDDLE,
    POS_TEXT_TOP,

    WIDTH_TEXT_LINE,
    WIDTH_TEXT_HEIGHT
};


class PdfFloatingToolbox {
public:
    PdfFloatingToolbox(MainWindow* theMainWindow, GtkOverlay* overlay);
    PdfFloatingToolbox& operator=(const PdfFloatingToolbox&) = delete;
    PdfFloatingToolbox(const PdfFloatingToolbox&) = delete;
    PdfFloatingToolbox& operator=(PdfFloatingToolbox&&) = delete;
    PdfFloatingToolbox(PdfFloatingToolbox&&) = delete;
    ~PdfFloatingToolbox() = default;

public:
    void show(int x, int y, PdfElemSelection* pSelection);
    void hide();
    void hideAndSelectionNullPtr();

    void postAction();
    bool getIsHidden() const;

private:
    void show();

    static gboolean getOverlayPosition(GtkOverlay* overlay, GtkWidget* widget, GdkRectangle* allocation,
                                       PdfFloatingToolbox* self);

    static void switchSelectTypeCb(GtkButton* button, PdfFloatingToolbox* pft);
    static void strikethroughCb(GtkButton* button, PdfFloatingToolbox* pft);
    static void underlineCb(GtkButton* button, PdfFloatingToolbox* pft);
    static void copyTextCb(GtkButton* button, PdfFloatingToolbox* pft);
    static void highlightCb(GtkButton* button, PdfFloatingToolbox* pft);
    static void closeCb(GtkButton* button, PdfFloatingToolbox* pft);

    void copyText();
    void createStrokes(PdfMarkerStyle position, PdfMarkerStyle width, int markerOpacity);
    void createStrokesForStrikethrough();
    void createStrokesForUnderline();
    void createStrokesForHighlight();


private:
    long unsigned int selectionPage = npos;
    bool isHidden;

    GtkWidget* floatingToolbox;
    MainWindow* theMainWindow;

    PdfElemSelection* pdfElemSelection = nullptr;

    int floatingToolboxX = 0;
    int floatingToolboxY = 0;
};
