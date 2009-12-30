#ifndef CUSTOMEVENTDEFINEGUI_H
#define CUSTOMEVENTDEFINEGUI_H

#include <QtGui/QDialog>


class NLSimple;
class QButtonGroup;
namespace Ui { class CustomEventDefineGui; }

class CustomEventDefineGui : public QDialog {
    Q_OBJECT
public:
    CustomEventDefineGui(NLSimple *&model, QWidget *parent = 0);
    ~CustomEventDefineGui();

protected:
    void changeEvent(QEvent *e);

private:
    Ui::CustomEventDefineGui *m_ui;
    NLSimple      *&m_model;
    QButtonGroup *m_buttonGroup;

    enum
    {
      kIndependantEffectId,
      kMultiplyInsulinId,
      kMultiplyCarbConsumedId
    };//enum

private slots:
  void cancel();
  void enableOkButton();
  void addEventDefToModel();
};

#endif // CUSTOMEVENTDEFINEGUI_H
