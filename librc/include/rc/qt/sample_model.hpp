// rc/qt/sample_model.hpp
//
// A list model that says what changed, from lesson 11-05, graduated.
//
// A JavaScript push onto a QStringList property wrote back and onto a QVariantList did not; a model that changed without the begin and end calls had the right row count and a Repeater that still showed the old count; with them one rowsInserted for each row and
// the Repeater followed, and clear gave one reset and an empty Repeater.

#ifndef RC_QT_SAMPLE_MODEL_HPP
#define RC_QT_SAMPLE_MODEL_HPP

#include <QtCore/QAbstractListModel>
#include <QtCore/QByteArray>
#include <QtCore/QHash>
#include <QtCore/QModelIndex>
#include <QtCore/QVariant>
#include <vector>

namespace rc {
namespace qt {

// A list in QML is shown by a view, and a view is told what changed by a signal. A property that is a list gives QML a copy of it. A JavaScript push onto the copy has
// different results for different list types, and none of them tells a view. A model is the way to say what happened: rows were inserted here, the whole list was reset.
//
// A model that changes its rows without the begin and end calls around the change is the worst case, because it looks right: rowCount() is correct, data() is correct, and
// every view that is already showing the model is still showing the old rows.

class SampleModel : public QAbstractListModel {
  Q_OBJECT

 public:
  enum Role { ValueRole = Qt::UserRole + 1, PositionRole };

  explicit SampleModel(QObject* parent = nullptr) : QAbstractListModel(parent) {}

  int rowCount(const QModelIndex& parent = QModelIndex()) const override {
    return parent.isValid() ? 0 : static_cast<int>(values_.size());
  }

  // The value of a row, or its position, by role. Anything that is not a row of this model, or a role it does not know, is an empty QVariant.
  QVariant data(const QModelIndex& index, int role) const override {
    if (!index.isValid() || index.parent().isValid() || index.row() < 0 || index.row() >= static_cast<int>(values_.size())) return QVariant();
    if (role == ValueRole) return values_[static_cast<std::size_t>(index.row())];
    if (role == PositionRole) return index.row();
    return QVariant();
  }

  // The names a QML delegate uses for the roles: `value` and `position`.
  QHash<int, QByteArray> roleNames() const override {
    QHash<int, QByteArray> names;
    names[ValueRole] = "value";
    names[PositionRole] = "position";
    return names;
  }

  // Adds a row at the end, and tells every view about exactly that row.
  Q_INVOKABLE void append(double value) {
    const int row = static_cast<int>(values_.size());
    beginInsertRows(QModelIndex(), row, row);
    values_.push_back(value);
    endInsertRows();
  }

  // Removes every row, and tells every view that the whole list is new.
  Q_INVOKABLE void clear() {
    beginResetModel();
    values_.clear();
    endResetModel();
  }

 private:
  std::vector<double> values_;
};
}  // namespace qt
}  // namespace rc

#endif  // RC_QT_SAMPLE_MODEL_HPP
