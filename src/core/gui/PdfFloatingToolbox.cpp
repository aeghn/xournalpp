#include "PdfFloatingToolbox.h"

#include "control/Control.h"
#include "gui/PageView.h"
#include "model/Stroke.h"
#include "undo/GroupUndoAction.h"
#include "undo/InsertUndoAction.h"

#include "GladeGui.h"
#include "MainWindow.h"

PdfFloatingToolbox::PdfFloatingToolbox(MainWindow* theMainWindow, GtkOverlay* overlay):
        isHidden(false), theMainWindow(theMainWindow) {
    this->floatingToolbox = theMainWindow->get("pdfFloatingToolbox");

    gtk_overlay_add_overlay(overlay, this->floatingToolbox);
    gtk_overlay_set_overlay_pass_through(overlay, this->floatingToolbox, true);

    g_signal_connect(overlay, "get-child-position", G_CALLBACK(this->getOverlayPosition), this);

    g_signal_connect(theMainWindow->get("pdfTbHighlight"), "clicked", G_CALLBACK(this->highlightCb), this);
    g_signal_connect(theMainWindow->get("pdfTbCopyText"), "clicked", G_CALLBACK(this->copyTextCb), this);
    g_signal_connect(theMainWindow->get("pdfTbUnderline"), "clicked", G_CALLBACK(this->underlineCb), this);
    g_signal_connect(theMainWindow->get("pdfTbStrikethrough"), "clicked", G_CALLBACK(this->strikethroughCb), this);
    g_signal_connect(theMainWindow->get("pdfTbDoNothing"), "clicked", G_CALLBACK(this->closeCb), this);
    g_signal_connect(theMainWindow->get("pdfTbChangeType"), "clicked", G_CALLBACK(this->switchSelectTypeCb), this);

    this->hideAndSelectionNullPtr();
}

void PdfFloatingToolbox::show(int x, int y, PdfElemSelection* pSelection) {
    this->pdfElemSelection = pSelection;
    this->floatingToolboxX = x;
    this->floatingToolboxY = y;
    this->show();
}

void PdfFloatingToolbox::hideAndSelectionNullPtr() {
    this->pdfElemSelection = nullptr;
    this->hide();
}

void PdfFloatingToolbox::hide() {
    if (this->isHidden)
        return;

    gtk_widget_hide(this->floatingToolbox);
    this->isHidden = true;
}

auto PdfFloatingToolbox::getOverlayPosition(GtkOverlay* overlay, GtkWidget* widget, GdkRectangle* allocation,
                                            PdfFloatingToolbox* self) -> gboolean {
    if (widget == self->floatingToolbox) {
        gtk_widget_get_allocation(widget, allocation);  // get existing width and height

        GtkRequisition natural;
        gtk_widget_get_preferred_size(widget, nullptr, &natural);
        allocation->width = natural.width;
        allocation->height = natural.height;

        if (self->pdfElemSelection && self->pdfElemSelection->getPageView()) {
            // Make sure the "pdfFloatingToolbox" is fully displayed.
            int gap = 5;

            GtkWidget* widget = self->theMainWindow->getWindow();
            GtkAllocation* alloc = g_new(GtkAllocation, 1);
            gtk_widget_get_allocation(gtk_widget_get_toplevel(widget), alloc);

            bool rightOK = self->floatingToolboxX + allocation->width + gap <= alloc->width;
            bool bottomOK = self->floatingToolboxY + allocation->height + gap <= alloc->height;

            allocation->x = rightOK ? self->floatingToolboxX + gap : self->floatingToolboxX - allocation->width - gap;
            allocation->y = bottomOK ? self->floatingToolboxY + gap : self->floatingToolboxY - allocation->height - gap;

            g_free(alloc);
        } else {
            allocation->x = self->floatingToolboxX;
            allocation->y = self->floatingToolboxY;
        }

        return true;
    }

    return false;
}

void PdfFloatingToolbox::postAction() {
    if (this->pdfElemSelection) {
        auto view = this->pdfElemSelection->getPageView();

        delete this->pdfElemSelection;
        this->pdfElemSelection = nullptr;

        view->pdfElemSelection = nullptr;
        view->rerenderPage();
    }

    this->hide();
}

void PdfFloatingToolbox::highlightCb(GtkButton* button, PdfFloatingToolbox* pft) {
    pft->createStrokesForHighlight();
    pft->postAction();
}

void PdfFloatingToolbox::copyTextCb(GtkButton* button, PdfFloatingToolbox* pft) {
    pft->copyText();
    pft->postAction();
}

void PdfFloatingToolbox::underlineCb(GtkButton* button, PdfFloatingToolbox* pft) {
    pft->createStrokesForUnderline();
    pft->postAction();
}

void PdfFloatingToolbox::strikethroughCb(GtkButton* button, PdfFloatingToolbox* pft) {
    pft->createStrokesForStrikethrough();
    pft->postAction();
}

void PdfFloatingToolbox::closeCb(GtkButton* button, PdfFloatingToolbox* pft) { pft->postAction(); }

void PdfFloatingToolbox::show() {
    this->isHidden = false;
    gtk_widget_hide(this->floatingToolbox);  // force showing in new position
    gtk_widget_show_all(this->floatingToolbox);
}

void PdfFloatingToolbox::copyText() {
    GtkClipboard* clipboard = gtk_widget_get_clipboard(this->theMainWindow->getWindow(), GDK_SELECTION_CLIPBOARD);

    const gchar* text = this->pdfElemSelection->getSelectedText().c_str();

    if (!text || strlen(text) == 0)
        return;

    gtk_clipboard_set_text(clipboard, text, -1);
}

void PdfFloatingToolbox::createStrokes(PdfMarkerStyle position, PdfMarkerStyle width, int markerOpacity) {
    uint64_t currentPage = theMainWindow->getXournal()->getCurrentPage();
    if (currentPage != this->pdfElemSelection->getSelectionPageNr()) {
        return;
    }

    const auto textRects = this->pdfElemSelection->getSelectedTextRects();
    if (textRects.empty())
        return;

    auto* control = this->theMainWindow->getControl();
    PageRef page = control->getCurrentPage();
    Layer* layer = page->getSelectedLayer();

    auto color = theMainWindow->getXournal()->getControl()->getToolHandler()->getColor();

    std::vector<Stroke*> strokes;
    for (XojPdfRectangle rect: textRects) {
        const double topOfLine = std::min(rect.y1, rect.y2);
        const double middleOfLine = (rect.y1 + rect.y2) / 2;
        const double bottomOfLine = std::max(rect.y1, rect.y2);
        const double rectWidth = std::abs(rect.y2 - rect.y1);

        // the center line position of stroke
        const double h = position == PdfMarkerStyle::POS_TEXT_BOTTOM ? bottomOfLine :
                         position == PdfMarkerStyle::POS_TEXT_MIDDLE ? middleOfLine :
                                                                       topOfLine;
        // the width of stroke
        const double w = width == PdfMarkerStyle::WIDTH_TEXT_LINE ? 1 : rectWidth;

        auto* stroke = new Stroke();
        stroke->setColor(color);
        stroke->setFill(markerOpacity);
        stroke->setToolType(STROKE_TOOL_HIGHLIGHTER);
        stroke->setWidth(w);
        stroke->addPoint(Point(rect.x1, h, -1));
        stroke->addPoint(Point(rect.x2, h, -1));
        stroke->setStrokeCapStyle(StrokeCapStyle::BUTT);

        strokes.push_back(stroke);
        layer->addElement(stroke);
        page->fireElementChanged(stroke);
    }

    auto undoAct = std::make_unique<GroupUndoAction>();
    auto* view = theMainWindow->getXournal()->getViewFor(control->getCurrentPageNo());
    for (auto&& stroke: strokes) {
        undoAct->addAction(std::make_unique<InsertUndoAction>(page, layer, stroke));
        view->rerenderElement(stroke);
    }
    control->getUndoRedoHandler()->addUndoAction(std::move(undoAct));
}

void PdfFloatingToolbox::switchSelectTypeCb(GtkButton* button, PdfFloatingToolbox* pft) {
    ToolType type = pft->theMainWindow->getControl()->getToolHandler()->getToolType();

    type = type == ToolType::TOOL_SELECT_PDF_TEXT_LINEAR ? ToolType::TOOL_SELECT_PDF_TEXT_RECT :
                                                           ToolType::TOOL_SELECT_PDF_TEXT_LINEAR;

    pft->theMainWindow->getControl()->selectTool(type);

    pft->pdfElemSelection->setToolType(type);
    pft->pdfElemSelection->selectFinally();
    pft->pdfElemSelection->getPageView()->repaintPage();
}


void PdfFloatingToolbox::createStrokesForHighlight() {
    this->createStrokes(PdfMarkerStyle::POS_TEXT_MIDDLE, PdfMarkerStyle::WIDTH_TEXT_HEIGHT, 60);
}

void PdfFloatingToolbox::createStrokesForUnderline() {
    this->createStrokes(PdfMarkerStyle::POS_TEXT_BOTTOM, PdfMarkerStyle::WIDTH_TEXT_LINE, 230);
}

void PdfFloatingToolbox::createStrokesForStrikethrough() {
    this->createStrokes(PdfMarkerStyle::POS_TEXT_MIDDLE, PdfMarkerStyle::WIDTH_TEXT_LINE, 230);
}

bool PdfFloatingToolbox::getIsHidden() const { return isHidden; }
