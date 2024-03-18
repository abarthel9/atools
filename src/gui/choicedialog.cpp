/*****************************************************************************
* Copyright 2015-2024 Alexander Barthel alex@littlenavmap.org
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*****************************************************************************/

#include "choicedialog.h"
#include "ui_choicedialog.h"
#include "gui/helphandler.h"
#include "gui/widgetstate.h"
#include  "settings/settings.h"

#include <QMimeData>
#include <QPushButton>
#include <QClipboard>
#include <QDebug>
#include <QTextDocumentFragment>
#include <QCheckBox>

namespace atools {
namespace gui {

const static char ID_PROPERTY[] = "checkboxid";

ChoiceDialog::ChoiceDialog(QWidget *parent, const QString& title, const QString& description, const QString& settingsPrefixParam,
                           const QString& helpBaseUrlParam)
  : QDialog(parent), ui(new Ui::ChoiceDialog), helpBaseUrl(helpBaseUrlParam), settingsPrefix(settingsPrefixParam)
{
  ui->setupUi(this);
  setWindowTitle(title);
  setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
  setWindowModality(Qt::ApplicationModal);

  ui->labelChoiceDescription->setVisible(!description.isEmpty());
  ui->labelChoiceDescription->setText(description);

  if(helpBaseUrl.isEmpty())
    // Remove help button if not requested
    ui->buttonBoxChoice->removeButton(ui->buttonBoxChoice->button(QDialogButtonBox::Help));

  ui->buttonBoxChoice->button(QDialogButtonBox::Ok)->setDefault(true);

  connect(ui->buttonBoxChoice, &QDialogButtonBox::clicked, this, &ChoiceDialog::buttonBoxClicked);
  updateButtonBoxState();
}

ChoiceDialog::~ChoiceDialog()
{
  // Save only dialog dimensions
  atools::gui::WidgetState widgetState(settingsPrefix, false);
  widgetState.save(this);

  delete ui;
}

void ChoiceDialog::addCheckBoxHiddenInt(int id)
{
  addCheckBoxInt(id, QString(), QString(), false /* checked*/, true /* disabled */, true /* hidden */);
}

void ChoiceDialog::addCheckBoxDisabledInt(int id, const QString& text, const QString& tooltip, bool checked)
{
  addCheckBoxInt(id, text, tooltip, checked, true /* disabled */, false /* hidden */);
}

void ChoiceDialog::addCheckBoxInt(int id, const QString& text, const QString& tooltip, bool checked, bool disabled,
                                  bool hidden)
{
  QCheckBox *checkBox = new QCheckBox(text, this);
  checkBox->setToolTip(tooltip);
  checkBox->setStatusTip(tooltip);
  checkBox->setProperty(ID_PROPERTY, id);
  checkBox->setChecked(checked);
  checkBox->setDisabled(disabled);
  checkBox->setHidden(hidden);
  index.insert(id, checkBox);
  connect(checkBox, &QCheckBox::toggled, this, &ChoiceDialog::checkBoxToggledInternal);

  // Add widget before the button box and verticalSpacerChoice
  ui->verticalLayoutScrollArea->insertWidget(-1, checkBox);
}

void ChoiceDialog::addLine()
{
  QFrame *line;
  line = new QFrame(this);
  line->setFrameShape(QFrame::HLine);
  line->setFrameShadow(QFrame::Sunken);
  ui->verticalLayoutScrollArea->insertWidget(-1, line);
}

void ChoiceDialog::addLabel(const QString& text)
{
  ui->verticalLayoutScrollArea->insertWidget(-1, new QLabel(text, this));
}

void ChoiceDialog::addSpacer()
{
  ui->verticalLayoutScrollArea->addSpacerItem(new QSpacerItem(10, 10, QSizePolicy::Minimum, QSizePolicy::Expanding)); // Move button up with spacer
}

const QVector<std::pair<int, bool> > ChoiceDialog::getCheckState() const
{
  QVector<std::pair<int, bool> > ids;
  for(QCheckBox *checkbox : index)
    ids.append(std::make_pair(checkbox->property(ID_PROPERTY).toInt(), checkbox->isChecked()));
  return ids;
}

bool ChoiceDialog::isCheckedInt(int id) const
{
  return index.value(id)->isChecked() && index.value(id)->isEnabled();
}

QCheckBox *ChoiceDialog::getCheckBoxInt(int id)
{
  return index.value(id);
}

void ChoiceDialog::buttonBoxClicked(QAbstractButton *button)
{
  QDialogButtonBox::StandardButton buttonType = ui->buttonBoxChoice->standardButton(button);

  if(buttonType == QDialogButtonBox::Ok)
  {
    saveState();
    accept();
  }
  else if(buttonType == QDialogButtonBox::Cancel)
    reject();
  else if(buttonType == QDialogButtonBox::Help)
  {
    if(!helpBaseUrl.isEmpty())
      atools::gui::HelpHandler::openHelpUrlWeb(this, helpOnlineUrl + helpBaseUrl, helpLanguageOnline);
  }
}

void ChoiceDialog::checkBoxToggledInternal(bool checked)
{
  updateButtonBoxState();
  QCheckBox *checkBox = dynamic_cast<QCheckBox *>(sender());
  if(checkBox != nullptr)
    emit checkBoxToggled(checkBox->property(ID_PROPERTY).toInt(), checked);
}

void ChoiceDialog::restoreState()
{
  atools::gui::WidgetState widgetState(settingsPrefix, false);
  widgetState.restore(this);

  // Restore checkboxes
  QStringList ids = atools::settings::Settings::instance().valueStrList(settingsPrefix + "CheckBoxStates");
  for(int i = 0; i < ids.size(); i += 2)
  {
    int id = ids.at(i).toInt();
    bool checked = ids.at(i + 1).toInt() > 0;

    if(index.value(id) != nullptr)
    {
      index.value(id)->setChecked(checked);
      emit checkBoxToggled(id, checked);
    }
  }
  updateButtonBoxState();
}

void ChoiceDialog::saveState() const
{
  atools::gui::WidgetState widgetState(settingsPrefix, false);
  widgetState.save(this);

  // Save checkboxes in a list of id 1, checked 1, id 2, checked 2, ...
  QStringList ids;
  for(const std::pair<int, bool>& state : getCheckState())
    ids << QString::number(state.first) << QString::number(state.second);

  atools::settings::Settings::instance().setValue(settingsPrefix + "CheckBoxStates", ids);
}

void ChoiceDialog::updateButtonBoxState()
{
  if(!required.isEmpty())
  {
    bool found = false;
    for(int i : required)
    {
      if(index.contains(i) && index.value(i)->isChecked())
        found = true;
    }

    ui->buttonBoxChoice->button(QDialogButtonBox::Ok)->setEnabled(found);
  }
  else
    ui->buttonBoxChoice->button(QDialogButtonBox::Ok)->setEnabled(true);
}

} // namespace gui
} // namespace atools
