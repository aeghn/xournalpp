#include "PdfFloatingToolbox.h"

#include "GladeGui.h"
#include "MainWindow.h"

PdfFloatingToolbox::PdfFloatingToolbox(MainWindow* theMainWindow, GtkOverlay* overlay) {
    auto settings = gtk_settings_get_for_screen(gdk_screen_get_default());

    auto vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    auto hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    this->mainWindow = theMainWindow;
    this->floatingToolbox = theMainWindow->get("pdfFloatingToolbox");

    gtk_overlay_add_overlay(overlay, this->floatingToolbox);
    gtk_overlay_set_overlay_pass_through(overlay, this->floatingToolbox, true);

    g_signal_connect(overlay, "get-child-position", G_CALLBACK(this->getOverlayPosition), this);

    g_signal_connect(theMainWindow->get("pdfTbHighlight"), "clicked", G_CALLBACK(this->highlightCb), this);
    g_signal_connect(theMainWindow->get("pdfTbCopyText"), "clicked", G_CALLBACK(this->copyTextCb), this);
    g_signal_connect(theMainWindow->get("pdfTbUnderline"), "clicked", G_CALLBACK(this->underlineCb), this);
    g_signal_connect(theMainWindow->get("pdfTbStrikethrough"), "clicked", G_CALLBACK(this->strikethroughCb), this);
    g_signal_connect(theMainWindow->get("pdfTbDoNothing"), "clicked", G_CALLBACK(this->closeCb), this);

    this->hide();
}

PdfFloatingToolbox::~PdfFloatingToolbox() = default;

void PdfFloatingToolbox::show(int x, int y, PDFTextSelectControl* pdfTextSelectControl) {
    delete this->pdfTextSelectControl;
    this->pdfTextSelectControl = pdfTextSelectControl;
    this->floatingToolboxX = x;
    this->floatingToolboxY = y;
    this->show();
}

void PdfFloatingToolbox::hide() {
    gtk_widget_hide(this->floatingToolbox);
}

auto PdfFloatingToolbox::getOverlayPosition(GtkOverlay* overlay, GtkWidget* widget, GdkRectangle* allocation,
                                            PdfFloatingToolbox* self) -> gboolean {
    if (widget == self->floatingToolbox) {
        gtk_widget_get_allocation(widget, allocation);  // get existing width and height

        GtkRequisition natural;
        gtk_widget_get_preferred_size(widget, nullptr, &natural);
        allocation->width = natural.width;
        allocation->height = natural.height;

        // show centered on x,y
        allocation->x = self->floatingToolboxX - allocation->width / 2;
        allocation->y = self->floatingToolboxY - allocation->height / 2;
        return true;
    }

    return false;
}


void PdfFloatingToolbox::highlightCb(GtkButton* button, PdfFloatingToolbox* pft) {
    pft->hide();
    pft->pdfTextSelectControl->drawHighlight();
}

void PdfFloatingToolbox::copyTextCb(GtkButton* button, PdfFloatingToolbox* pft) {
    pft->hide();
    pft->pdfTextSelectControl->copyText();
}

void PdfFloatingToolbox::underlineCb(GtkButton* button, PdfFloatingToolbox* pft) {
    pft->hide();
    pft->pdfTextSelectControl->drawUnderline();
}

void PdfFloatingToolbox::strikethroughCb(GtkButton* button, PdfFloatingToolbox* pft) {
    pft->hide();
    pft->pdfTextSelectControl->drawStrikethrough();
}

void PdfFloatingToolbox::closeCb(GtkButton* button, PdfFloatingToolbox* pft) {
    pft->hide();
    pft->pdfTextSelectControl->rerender();
}

void PdfFloatingToolbox::show() {
    gtk_widget_hide(this->floatingToolbox);  // force showing in new position
    gtk_widget_show_all(this->floatingToolbox);
}