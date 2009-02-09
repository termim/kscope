/***************************************************************************
 *   Copyright (C) 2007-2009 by Elad Lahav
 *   elad_lahav@users.sourceforge.net
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 ***************************************************************************/

#include "textfilterdialog.h"

namespace KScope
{

namespace Core
{

TextFilterDialog::TextFilterDialog(const QRegExp& re, QWidget* parent)
	: QDialog(parent), Ui::TextFilterDialog()
{
	setupUi(this);

	patternEdit_->setText(re.pattern());
	switch (re.patternSyntax()) {
	case QRegExp::FixedString:
		stringButton_->setChecked(true);
		break;

	case QRegExp::RegExp:
	case QRegExp::RegExp2:
		regExpButton_->setChecked(true);
		break;

	case QRegExp::Wildcard:
		simpRegExpButton_->setChecked(true);
		break;
	}
}

TextFilterDialog::~TextFilterDialog()
{
}

QRegExp TextFilterDialog::filter() const
{
	QRegExp re;

	if (stringButton_->isChecked())
		re.setPatternSyntax(QRegExp::FixedString);
	else if (regExpButton_->isChecked())
		re.setPatternSyntax(QRegExp::RegExp);
	else if (simpRegExpButton_->isChecked())
		re.setPatternSyntax(QRegExp::Wildcard);

	re.setPattern(patternEdit_->text());

	return re;
}

} // namespace Core

} // namespace KScope
