/*
    This file is part of Cute Chess.

    Cute Chess is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Cute Chess is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Cute Chess.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "newgamedlg.h"
#include "ui_newgamedlg.h"

#include <QAbstractItemView>

#include <board/boardfactory.h>
#include <enginemanager.h>

#include "engineconfigurationmodel.h"
#include "engineconfigproxymodel.h"
#include "engineconfigurationdlg.h"
#include "timecontroldlg.h"
#include "stringvalidator.h"

#include <modeltest.h>

NewGameDialog::NewGameDialog(EngineManager* engineManager, QWidget* parent)
	: QDialog(parent),
	  m_engineManager(engineManager),
	  ui(new Ui::NewGameDialog)
{
	Q_ASSERT(engineManager != nullptr);
	ui->setupUi(this);

	m_engines = new EngineConfigurationModel(m_engineManager, this);
	#ifdef QT_DEBUG
	new ModelTest(m_engines, this);
	#endif

	connect(ui->m_configureWhiteEngineButton, SIGNAL(clicked()),
		this, SLOT(configureEngine()));
	connect(ui->m_configureBlackEngineButton, SIGNAL(clicked()),
		this, SLOT(configureEngine()));

	m_proxyModel = new EngineConfigurationProxyModel(this);
	m_proxyModel->setSourceModel(m_engines);
	m_proxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
	m_proxyModel->sort(0);
	m_proxyModel->setDynamicSortFilter(true);

	StringValidator* engineValidator = new StringValidator(this);
	engineValidator->setModel(m_proxyModel);

	connect(ui->m_whiteEngineComboBox, SIGNAL(currentIndexChanged(int)),
		this, SLOT(onEngineChanged(int)));
	connect(ui->m_blackEngineComboBox, SIGNAL(currentIndexChanged(int)),
		this, SLOT(onEngineChanged(int)));

	ui->m_whiteEngineComboBox->setModel(m_proxyModel);
	ui->m_whiteEngineComboBox->setValidator(engineValidator);
	ui->m_blackEngineComboBox->setModel(m_proxyModel);
	ui->m_blackEngineComboBox->setValidator(engineValidator);

	connect(ui->m_gameSettings, SIGNAL(variantChanged(QString)),
		this, SLOT(onVariantChanged(QString)));
	onVariantChanged(ui->m_gameSettings->chessVariant());
}

NewGameDialog::~NewGameDialog()
{
	delete ui;
}

NewGameDialog::PlayerType NewGameDialog::playerType(Chess::Side side) const
{
	Q_ASSERT(!side.isNull());

	if (side == Chess::Side::White)
		return (ui->m_whitePlayerHumanRadio->isChecked()) ? Human : CPU;
	else
		return (ui->m_blackPlayerHumanRadio->isChecked()) ? Human : CPU;
}

EngineConfiguration NewGameDialog::engineConfig(Chess::Side side) const
{
	return m_engineConfig[side];
}

QString NewGameDialog::selectedVariant() const
{
	return ui->m_gameSettings->chessVariant();
}

TimeControl NewGameDialog::timeControl() const
{
	return ui->m_gameSettings->timeControl();
}

void NewGameDialog::configureEngine()
{
	Chess::Side side;
	if (QObject::sender() == ui->m_configureWhiteEngineButton)
		side = Chess::Side::White;
	else
		side = Chess::Side::Black;

	EngineConfigurationDialog dlg(EngineConfigurationDialog::ConfigureEngine, this);
	dlg.applyEngineInformation(m_engineConfig[side]);

	if (dlg.exec() == QDialog::Accepted)
		m_engineConfig[side] = dlg.engineConfiguration();
}

void NewGameDialog::onVariantChanged(const QString& variant)
{
	m_proxyModel->setFilterVariant(variant);

	bool empty = m_proxyModel->rowCount() == 0;
	if (empty)
	{
		ui->m_whitePlayerHumanRadio->setChecked(true);
		ui->m_blackPlayerHumanRadio->setChecked(true);
	}

	ui->m_whitePlayerCpuRadio->setDisabled(empty);
	ui->m_blackPlayerCpuRadio->setDisabled(empty);
}

void NewGameDialog::onEngineChanged(int index, Chess::Side side)
{
	if (side == Chess::Side::NoSide)
	{
		if (QObject::sender() == ui->m_whiteEngineComboBox)
			side = Chess::Side::White;
		else
			side = Chess::Side::Black;
	}

	auto modelIndex = m_proxyModel->index(index, 0);
	if (modelIndex.isValid())
	{
		int i = m_proxyModel->mapToSource(modelIndex).row();
		m_engineConfig[side] = m_engineManager->engineAt(i);
	}
}
