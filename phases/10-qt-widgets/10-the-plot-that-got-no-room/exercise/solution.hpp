#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <QtWidgets/QSizePolicy>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

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
    // TODO: toolbar_->setFixedHeight(30) and status_->setFixedHeight(20). plot_->setMinimumSize(120, 80) and plot_->setSizePolicy(Expanding, Expanding).
    // Then a vertical layout on this widget: QVBoxLayout* layout = new QVBoxLayout(this); no margins (setContentsMargins(0, 0, 0, 0)) and no spacing
    // (setSpacing(0)); add the toolbar, then the plot with a stretch factor of 1 (layout->addWidget(plot_, 1)), then the status bar.
  }

  QWidget* toolbar() const { return toolbar_; }
  QWidget* plot() const { return plot_; }
  QWidget* status() const { return status_; }

 private:
  QWidget* toolbar_;
  QWidget* plot_;
  QWidget* status_;
};

#endif  // LESSON_SOLUTION_HPP
