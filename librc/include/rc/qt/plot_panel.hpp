// rc/qt/plot_panel.hpp
//
// A chart panel whose plot takes the room, from lesson 10-10, graduated.
//
// Three plain widgets in a vertical layout shared 300 pixels equally, 100 each. With a fixed toolbar of 30, a fixed status bar of 20 and a plot that expands with a stretch of 1, the plot was 250 at 300 high and 450 at 500, the other two never moved,
// and hiding the status bar gave its 20 to the plot.

#ifndef RC_QT_PLOT_PANEL_HPP
#define RC_QT_PLOT_PANEL_HPP

#include <QtWidgets/QSizePolicy>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

namespace rc {
namespace qt {

// A chart panel is a toolbar across the top, a status bar across the bottom and the plot between them. When the window is resized, the toolbar and the status
// bar should stay the height they are and the plot should take all the rest. Left to its defaults, a layout shares the room out equally between the three,
// and the plot gets a third of the window.
//
// Three things say who gets the room: a widget with a fixed height keeps it, a widget with the Expanding size policy asks for more, and a stretch factor says how
// the rest is divided. And a minimum size on the plot is what stops the window being made too small to see it.

class PlotPanel : public QWidget {
 public:
  explicit PlotPanel(QWidget* parent = nullptr)
      : QWidget(parent), toolbar_(new QWidget(this)), plot_(new QWidget(this)), status_(new QWidget(this)) {
    toolbar_->setFixedHeight(30);
    status_->setFixedHeight(20);
    plot_->setMinimumSize(120, 80);
    plot_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(toolbar_);
    layout->addWidget(plot_, 1);
    layout->addWidget(status_);
  }

  QWidget* toolbar() const { return toolbar_; }
  QWidget* plot() const { return plot_; }
  QWidget* status() const { return status_; }

 private:
  QWidget* toolbar_;
  QWidget* plot_;
  QWidget* status_;
};
}  // namespace qt
}  // namespace rc

#endif  // RC_QT_PLOT_PANEL_HPP
