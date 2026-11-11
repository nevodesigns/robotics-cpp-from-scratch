// A widget cannot exist before a QApplication does, so the framework's own main is suppressed and one is written below.
#define RC_TEST_NO_MAIN
#include <rc/test/rc_test.hpp>

#include <QtCore/QCoreApplication>
#include <QtWidgets/QApplication>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

#include <cstdlib>
#include <iostream>

#include "solution.hpp"

namespace {

void settle() {
  QCoreApplication::processEvents();
  QCoreApplication::processEvents();
}

}  // namespace

RC_TEST("three plain widgets in a layout are given a third of the window each") {
  QWidget window;
  QWidget a(&window), b(&window), c(&window);
  QVBoxLayout layout(&window);
  layout.setContentsMargins(0, 0, 0, 0);
  layout.setSpacing(0);
  layout.addWidget(&a);
  layout.addWidget(&b);
  layout.addWidget(&c);
  window.resize(400, 300);
  window.show();
  settle();
  std::cout << "  plain widgets in a window 300 high: " << a.height() << ", " << b.height() << ", " << c.height() << "\n\n";
  RC_CHECK(std::abs(a.height() - 100) <= 1);
  RC_CHECK(std::abs(b.height() - 100) <= 1);
  RC_CHECK(std::abs(c.height() - 100) <= 1);

  // The panel of this lesson, in the same window size, is not divided that way.
  PlotPanel panel;
  panel.resize(400, 300);
  panel.show();
  settle();
  RC_CHECK_EQ(panel.plot()->height(), 250);
}

RC_TEST("the toolbar and the status bar keep their height and the plot takes the rest, however the window is resized") {
  PlotPanel panel;
  panel.resize(400, 300);
  panel.show();
  settle();
  const int toolbar300 = panel.toolbar()->height();
  const int status300 = panel.status()->height();
  const int plot300 = panel.plot()->height();
  panel.resize(400, 500);
  settle();
  const int toolbar500 = panel.toolbar()->height();
  const int status500 = panel.status()->height();
  const int plot500 = panel.plot()->height();
  std::cout << "  at 300 high: toolbar " << toolbar300 << ", plot " << plot300 << ", status " << status300 << "; at 500 high: toolbar " << toolbar500 << ", plot " << plot500
            << ", status " << status500 << "\n\n";
  RC_CHECK_EQ(toolbar300, 30);
  RC_CHECK_EQ(status300, 20);
  RC_CHECK_EQ(plot300, 250);
  RC_CHECK_EQ(toolbar500, 30);
  RC_CHECK_EQ(status500, 20);
  RC_CHECK_EQ(plot500, 450);
}

RC_TEST("the three are stacked in order with no gap, and the plot is as wide as the window") {
  PlotPanel panel;
  panel.resize(400, 300);
  panel.show();
  settle();
  RC_CHECK_EQ(panel.toolbar()->y(), 0);
  RC_CHECK_EQ(panel.plot()->y(), panel.toolbar()->height());
  RC_CHECK_EQ(panel.status()->y(), panel.plot()->y() + panel.plot()->height());
  RC_CHECK_EQ(panel.status()->y() + panel.status()->height(), panel.height());
  RC_CHECK_EQ(panel.plot()->width(), panel.width());
  RC_CHECK_EQ(panel.plot()->x(), 0);
  panel.resize(640, 300);
  settle();
  RC_CHECK_EQ(panel.plot()->width(), panel.width());
  RC_CHECK_EQ(panel.toolbar()->width(), panel.width());
}

RC_TEST("the window cannot be made smaller than the toolbar, the status bar and the smallest plot") {
  PlotPanel panel;
  const QSize hint = panel.minimumSizeHint();
  std::cout << "  the smallest the panel asks to be: " << hint.width() << " by " << hint.height() << "\n";
  RC_CHECK_EQ(hint.height(), 30 + 80 + 20);
  RC_CHECK(hint.width() >= 120);
  panel.resize(60, 40);
  panel.show();
  settle();
  std::cout << "  asked to be 60 by 40, it is " << panel.width() << " by " << panel.height() << "\n\n";
  RC_CHECK(panel.height() >= 130);
  RC_CHECK(panel.width() >= 120);
  RC_CHECK(panel.plot()->height() >= 80);
}

RC_TEST("a hidden status bar gives its room to the plot, and showing it takes the room back") {
  PlotPanel panel;
  panel.resize(400, 300);
  panel.show();
  settle();
  panel.status()->hide();
  settle();
  const int hidden_plot = panel.plot()->height();
  panel.status()->show();
  settle();
  const int shown_plot = panel.plot()->height();
  std::cout << "  status bar hidden: the plot is " << hidden_plot << " high, shown: " << shown_plot << "\n\n";
  RC_CHECK_EQ(hidden_plot, 270);
  RC_CHECK_EQ(shown_plot, 250);
}

int main(int argc, char** argv) {
  qputenv("QT_QPA_PLATFORM", "offscreen");
  QApplication application(argc, argv);
  return rc::test::run_all();
}
