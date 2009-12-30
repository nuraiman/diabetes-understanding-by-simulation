#include "CustomEventDefineGui.h"
#include "ui_CustomEventDefineGui.h"

#include <QPushButton>

#include <vector>
#include <algorithm>

#include "ResponseModel.hh"
#include "MiscGuiUtils.hh"
#include "ArtificialPancrease.hh"

using namespace std;

CustomEventDefineGui::CustomEventDefineGui(NLSimple *&model,  QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::CustomEventDefineGui),
    m_model(model)
{
  m_ui->setupUi(this);
  if( !m_model ) done(0);

  //make the dependancy buttons mutally exclusive
  //IndependantEffect, MultiplyInsulin, MultiplyCarbConsumed defined
  //  in ResponseModel.hh
  m_buttonGroup = new QButtonGroup( this );
  m_buttonGroup->addButton( m_ui->constantButton, IndependantEffect );
  m_buttonGroup->addButton( m_ui->insulinButton,  MultiplyInsulin );
  m_buttonGroup->addButton( m_ui->carbButton,     MultiplyCarbConsumed );
  m_buttonGroup->setExclusive(true);
  m_ui->constantButton->setChecked(true);


  //Make sure only non-already defined ID numbers are in the drop down box
  using namespace boost;
  vector<int> defTypes;
  for( int i = 1; i < 11; ++i ) defTypes.push_back( i );

  vector<int>::iterator pos;
  NLSimple::EventDefIter iter = m_model->m_customEventDefs.begin();
  for( ; iter != m_model->m_customEventDefs.end(); ++iter )
  {
    pos = std::find( defTypes.begin(), defTypes.end(), iter->first );
    if( pos != defTypes.end() ) defTypes.erase( pos );
  }//for(...)

  for( size_t i = 0; i < defTypes.size(); ++i )
  {
    string numStr = lexical_cast<string>(defTypes[i]);
    m_ui->idNumEdit->addItem( numStr.c_str(), defTypes[i] );
  }//for( loop over available id types )

  m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

  connect(m_ui->buttonBox, SIGNAL(rejected()), this, SLOT(cancel()) );
  connect(m_ui->buttonBox, SIGNAL(accepted()), this, SLOT(addEventDefToModel()));
}//CustomEventDefineGui constructor



CustomEventDefineGui::~CustomEventDefineGui()
{
    delete m_ui;
}//CustomEventDefineGui destructor


void CustomEventDefineGui::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        m_ui->retranslateUi(this);
        break;
    default:
        break;
    }
}//void CustomEventDefineGui::changeEvent(QEvent *e)


void CustomEventDefineGui::cancel()
{
  done(0);
}//void CustomEventDefineGui::cancel()

void CustomEventDefineGui::enableOkButton()
{
  const string name =  m_ui->nameEdit->text().toStdString();
  if( name.length() )
  {
    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
  }else
  {
    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    return;
  }//if( non-empty box ) / else

  //if name already taken, disable ok button
  NLSimple::EventDefIter iter = m_model->m_customEventDefs.begin();
  for( ; iter != m_model->m_customEventDefs.end(); ++iter )
  {
    if( iter->second.getName() == name )
    {
      m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
      return;
    }//if( name already taken )
  }//for( loop over defined EventDefs )
}//void CustomEventDefineGui::enableOkButton()

void CustomEventDefineGui::addEventDefToModel()
{
  const TimeDuration dur = qtTimeToDuration( m_ui->timeEdit->time() );
  const int selectedIndex = m_ui->idNumEdit->currentIndex();
  const int recordId = m_ui->idNumEdit->itemData(selectedIndex).toInt();
  const int nPoint = m_ui->numPointsEdit->value();
  const string name = m_ui->nameEdit->text().toStdString();

  EventDefType evType = (EventDefType)m_buttonGroup->checkedId();

  cout << "addEventDefToModel(): Creating a event type indicated by'" << recordId
       << "' with name " << name << " and duration " << dur << " with type '"
       << evType << "', and " << nPoint << " points" << endl;

  bool added = m_model->defineCustomEvent( recordId, name, dur, evType, nPoint);
  if( added ) done(1);
  return;
}//void CustomEventDefineGui::addEventDefToModel()


