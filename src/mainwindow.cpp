#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
	config = new wolfsuite::Config();
	setupConfig();

	ui.setupUi(this);

	fullscreen = false;
	maximized = false;

	vp = new wolfsuite::VideoParser(config->config.find("libraryfolder")->second);

	mainMenu = new QMenu(this);
	videoMenu = new QMenu("Video Track", this);
	audioMenu = new QMenu("Audio Track", this);
	aspectRatioMenu = new QMenu("Aspect Ratio", this);
	cropRatioMenu = new QMenu("Crop Ratio", this);
	subtitlesMenu = new QMenu("Subtitles", this);

	videoGroup = new QActionGroup(videoMenu);
	audioGroup = new QActionGroup(audioMenu);
	aspectRatioGroup = new QActionGroup(aspectRatioMenu);
	cropRatioGroup = new QActionGroup(aspectRatioMenu);
	subtitlesGroup = new QActionGroup(subtitlesMenu);

	defaultAR = new QAction(tr("Default"), this);
	firstAR = new QAction(tr("16:9"), this);
	secondAR = new QAction(tr("16:10"), this);
	thirdAR = new QAction(tr("4:3"), this);

	defaultCR = new QAction(tr("Default"), this);
	firstCR = new QAction(tr("16:9"), this);
	secondCR = new QAction(tr("16:10"), this);
	thirdCR = new QAction(tr("4:3"), this);

	allItem = new QListWidgetItem();
	currentItem = new QString(qvariant_cast<QString>(allItem->data(Qt::DisplayRole)));

	init();
}

MainWindow::~MainWindow() {
	delete vp;
	delete videoGroup;
	delete audioGroup;
	delete videoMenu;
	delete audioMenu;
	delete subtitlesMenu;
	delete mainMenu;
	//delete player;
}

void MainWindow::init() {
	ui.video->installEventFilter(this);
	ui.video->setMouseTracking(true);
	ui.videoList->setItemDelegate(new VideoListDelegate());

	ui.playlistList->setFrameShape(QFrame::NoFrame);
	ui.videoList->setIconSize(QSize(160, 90));

	connect(ui.video, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(showVideoMenu(const QPoint&)));
	connect(ui.videoList, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(videoDoubleClicked(QListWidgetItem*)));
	connect(ui.playlistList, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(playlistChanged(QListWidgetItem*)));
	connect(ui.sortingBox, SIGNAL(currentIndexChanged(int)), this, SLOT(handleSorting()));
	connect(ui.searchEdit, SIGNAL(textEdited(QString)), this, SLOT(handleSearch(QString)));

	ui.stackedWidget->setCurrentIndex(0);
	ui.playerControl->setLayout(ui.playerControls);

	ui.sortingBox->addItem("Ascending");
	ui.sortingBox->addItem("Descending");
	ui.sortingBox->setIconSize(QSize(20, 20));
	ui.sortingBox->setItemIcon(0, QPixmap(":/MainWindow/ascSort.png"));
	ui.sortingBox->setItemIcon(1, QPixmap(":/MainWindow/desSort.png"));
	ui.sortingBox->setCurrentIndex(0);

	videoGroup->setExclusive(true);
	audioGroup->setExclusive(true);
	aspectRatioGroup->setExclusive(true);
	cropRatioGroup->setExclusive(true);
	subtitlesGroup->setExclusive(true);

	mainMenu->addMenu(videoMenu);
	mainMenu->addMenu(audioMenu);
	mainMenu->addMenu(aspectRatioMenu);
	mainMenu->addMenu(cropRatioMenu);
	mainMenu->addMenu(subtitlesMenu);

	defaultAR->setCheckable(true);
	firstAR->setCheckable(true);
	secondAR->setCheckable(true);
	thirdAR->setCheckable(true);

	defaultCR->setCheckable(true);
	firstCR->setCheckable(true);
	secondCR->setCheckable(true);
	thirdCR->setCheckable(true);

	connect(defaultAR, &QAction::triggered, this, &MainWindow::changeAspectRatio);
	connect(firstAR, &QAction::triggered, this, &MainWindow::changeAspectRatio);
	connect(secondAR, &QAction::triggered, this, &MainWindow::changeAspectRatio);
	connect(thirdAR, &QAction::triggered, this, &MainWindow::changeAspectRatio);

	connect(defaultCR, &QAction::triggered, this, &MainWindow::changeCropRatio);
	connect(firstCR, &QAction::triggered, this, &MainWindow::changeCropRatio);
	connect(secondCR, &QAction::triggered, this, &MainWindow::changeCropRatio);
	connect(thirdCR, &QAction::triggered, this, &MainWindow::changeCropRatio);

	aspectRatioMenu->addAction(defaultAR);
	aspectRatioGroup->addAction(defaultAR);
	aspectRatioMenu->addAction(firstAR);
	aspectRatioGroup->addAction(firstAR);
	aspectRatioMenu->addAction(secondAR);
	aspectRatioGroup->addAction(secondAR);
	aspectRatioMenu->addAction(thirdAR);
	aspectRatioGroup->addAction(thirdAR);

	cropRatioMenu->addAction(defaultCR);
	cropRatioGroup->addAction(defaultCR);
	cropRatioMenu->addAction(firstCR);
	cropRatioGroup->addAction(firstCR);
	cropRatioMenu->addAction(secondCR);
	cropRatioGroup->addAction(secondCR);
	cropRatioMenu->addAction(thirdCR);
	cropRatioGroup->addAction(thirdCR);

	allItem->setData(Qt::DisplayRole, "Your Library");
	allItem->setIcon(QPixmap(":/MainWindow/folder.png"));
	currentItem = new QString(qvariant_cast<QString>(allItem->data(Qt::DisplayRole)));
	ui.playlistList->addItem(allItem);
	ui.playlistList->item(0)->setSelected(true);
	
	ui.playButton->setEnabled(false);
	ui.pauseButton->setEnabled(false);
	ui.stopButton->setEnabled(false);
	ui.deletePlaylistButton->setEnabled(false);

	updateVideoList();
	updatePlaylistList();
}

void MainWindow::setupConfig() {
	if (!config->configExists()) {
		QFileDialog *dialog = new QFileDialog(this);
		dialog->setWindowIconText("Select your library folder");
		QString dir = dialog->getExistingDirectory();
		config->createConfig(dir.toStdString());
		delete dialog;
	} else {
		config->loadConfig();
	}
}

void MainWindow::videoDoubleClicked(QListWidgetItem* item) {
	ui.stackedWidget->setCurrentIndex(1);
	setWindowTitle(qvariant_cast<QString>(item->data(Qt::DisplayRole)));

	QString file = item->statusTip();

	if (file.isEmpty())
		return;

	player = new wolfsuite::Player(file);

	player->player->setVideoWidget(ui.video);
	ui.video->setContextMenuPolicy(Qt::CustomContextMenu);

	connect(player->videoControl, SIGNAL(actions(QList<QAction *>, const Vlc::ActionsType)), this, SLOT(updateMenus(QList<QAction *>, const Vlc::ActionsType)));
	connect(player->audioControl, SIGNAL(actions(QList<QAction *>, const Vlc::ActionsType)), this, SLOT(updateMenus(QList<QAction *>, const Vlc::ActionsType)));
	connect(player->player, &VlcMediaPlayer::end, this, &MainWindow::on_stopButton_clicked);

	ui.video->setMediaPlayer(player->player);
	ui.volume->setMediaPlayer(player->player);
	ui.seek->setMediaPlayer(player->player);
	ui.volume->setVolume(50);

	defaultAR->setChecked(true);
	defaultAR->setEnabled(true);

	defaultCR->setChecked(true);
	defaultCR->setEnabled(true);

	ui.playButton->setEnabled(true);
	ui.playButton->click();
}

void MainWindow::on_playButton_clicked() {
	ui.playButton->setEnabled(false);
	ui.pauseButton->setEnabled(true);
	ui.stopButton->setEnabled(true);

	player->player->play();
}

void MainWindow::on_pauseButton_clicked() {
	ui.playButton->setEnabled(true);
	ui.pauseButton->setEnabled(false);
	ui.stopButton->setEnabled(true);

	player->player->pause();
}

void MainWindow::on_stopButton_clicked() {
	ui.playButton->setEnabled(true);
	ui.pauseButton->setEnabled(false);
	ui.stopButton->setEnabled(false);

	player->player->stop();
}

void MainWindow::on_decSpeedButton_clicked() {
	if (player->player->playbackRate() != 0.25)
		player->player->setPlaybackRate(player->player->playbackRate() / 2);
}

void MainWindow::on_incSpeedButton_clicked() {
	if (player->player->playbackRate() != 4)
		player->player->setPlaybackRate(player->player->playbackRate() * 2);
}

void MainWindow::on_stepButton_clicked() {
	if ((player->player->time() + 30000) < player->player->length())
		player->player->setTime(player->player->time() + 30000);
}

void MainWindow::on_volumeMuteButton_clicked() {
	if (ui.volume->mute()) {
		ui.volumeDownButton->setEnabled(true);
		ui.volumeUpButton->setEnabled(true);
		ui.volume->setMute(false);
	} else {
		ui.volumeDownButton->setEnabled(false);
		ui.volumeUpButton->setEnabled(false);
		ui.volume->setMute(true);
	}
}

void MainWindow::on_volumeDownButton_clicked() {
	if ((ui.volume->volume() - 10) < 0)
		ui.volume->setVolume(0);
	else
		ui.volume->setVolume(ui.volume->volume() - 10);
}

void MainWindow::on_volumeUpButton_clicked() {
	if ((ui.volume->volume() + 10) > 200)
		ui.volume->setVolume(200);
	else
		ui.volume->setVolume(ui.volume->volume() + 10);
}

void MainWindow::on_fullscreenButton_clicked() {
	if (!fullscreen)
		setFullscreen(true);
	else
		setFullscreen(false);
}

void MainWindow::on_backButton_clicked() {

	player->player->stop();

	videoMenu->clear();
	audioMenu->clear();
	subtitlesMenu->clear();

	// POTENTIALLY DANGEROUS
	player = NULL;

	ui.stackedWidget->setCurrentIndex(0);
	setWindowTitle("WolfSuite");
}

void MainWindow::on_refreshButton_clicked() {
	ui.searchEdit->clear();
	vp->scanFolder(qvariant_cast<QString>(ui.playlistList->selectedItems().at(0)->data(Qt::DisplayRole)));
	updateVideoList();
}

void MainWindow::on_addButton_clicked() {
	QStringList filename = QFileDialog::getOpenFileNames(this, tr("Add video(s) to your library"), "", tr("Video files (*.mkv *.mov *.wmv *.flv *.mp4 *.avi)"));

	if (filename.count() == 0)
		return;

	wolfsuite::CopyFile* cf = new wolfsuite::CopyFile(filename, QString::fromStdString(config->config.find("libraryfolder")->second) + "/");
	QProgressDialog* progress = new QProgressDialog(this, Qt::WindowSystemMenuHint | Qt::WindowTitleHint);
	progress->setWindowTitle("Adding " + QString::number(cf->copyList.count()) + " file(s)");
	progress->resize(progress->size() + QSize(40, 0));
	progress->setMinimum(0);
	progress->setMaximum(cf->copyList.count());
	progress->setValue(0);
	progress->setWindowModality(Qt::WindowModal);
	
	connect(cf, &wolfsuite::CopyFile::finished, progress, &QObject::deleteLater);
	connect(cf, &wolfsuite::CopyFile::finished, this, &MainWindow::on_refreshButton_clicked);
	connect(cf, &wolfsuite::CopyFile::finished, this, &MainWindow::handleSorting);
	connect(cf, &wolfsuite::CopyFile::finished, cf, &QObject::deleteLater);
	connect(cf, &wolfsuite::CopyFile::signalCopyFile, progress, &QProgressDialog::setValue);
	cf->start();
}

void MainWindow::on_deleteButton_clicked() {
	if (ui.videoList->currentItem() != NULL) {
		QMessageBox msgBox(this);
		msgBox.setWindowTitle("Delete File");
		msgBox.setText("Are you sure you want to delete " + ui.videoList->currentItem()->data(Qt::DisplayRole).toString() + "?");
		msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
		msgBox.setDefaultButton(QMessageBox::Cancel);
		msgBox.setIcon(QMessageBox::Question);
		if (msgBox.exec() == QMessageBox::Yes) {
			wolfsuite::RemoveFile* rf = new wolfsuite::RemoveFile(ui.videoList->currentItem()->data(Qt::StatusTipRole).toString());
			QMessageBox* loadingBox = new QMessageBox();
			loadingBox->setWindowTitle("Deleting file");
			loadingBox->setWindowFlags(Qt::Dialog);
			loadingBox->setWindowIcon(QIcon(":/MainWindow/wolf.png"));
			loadingBox->setText("Deleting file " + ui.videoList->currentItem()->data(Qt::DisplayRole).toString());
			loadingBox->setStandardButtons(0);

			connect(rf, &wolfsuite::RemoveFile::started, loadingBox, &QDialog::exec);
			connect(rf, &wolfsuite::RemoveFile::finished, loadingBox, &QDialog::accept);
			connect(rf, &wolfsuite::RemoveFile::finished, this, &MainWindow::on_refreshButton_clicked);
			connect(rf, &wolfsuite::RemoveFile::finished, loadingBox, &QObject::deleteLater);
			connect(rf, &wolfsuite::RemoveFile::finished, rf, &QObject::deleteLater);

			vp->deleteVideo(ui.videoList->currentItem()->data(Qt::StatusTipRole).toString());
			rf->start();
		}
	}
}

void MainWindow::on_infoButton_clicked() {
	if (ui.videoList->currentItem() != NULL) {
		EditInfo *info = new EditInfo(ui.videoList->currentItem());
		QPalette palette;
		palette.setColor(QPalette::Background, Qt::white);
		info->setAutoFillBackground(true);
		info->setPalette(palette);
		info->show();
		connect(info, &EditInfo::destroyed, this, &MainWindow::updateVideoList);
		connect(info, &EditInfo::destroyed, this, &MainWindow::handleSorting);
	}
}

void MainWindow::on_addPlaylistButton_clicked() {
	QString playlist = QInputDialog::getText(this, "Add Playlist", "Name new playlist:");
	if (!playlist.isEmpty()) {
		config->addPlaylist(playlist);
		updatePlaylistList();
	}
}

void MainWindow::on_deletePlaylistButton_clicked() {
	if (ui.playlistList->currentItem() != NULL) {
		QMessageBox msgBox(this);
		msgBox.setWindowTitle("Delete File");
		msgBox.setText("Are you sure you want to delete " + ui.playlistList->currentItem()->data(Qt::DisplayRole).toString() + "?");
		msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
		msgBox.setDefaultButton(QMessageBox::Cancel);
		msgBox.setIcon(QMessageBox::Question);
		if (msgBox.exec() == QMessageBox::Yes)
			config->deletePlaylist(qvariant_cast<QString>(ui.playlistList->currentItem()->data(Qt::DisplayRole)));
		updatePlaylistList();
		updateVideoList();
	}
}

void MainWindow::handleSorting() {
	switch (ui.sortingBox->currentIndex()) {
	case 0:
		ui.videoList->sortItems(Qt::AscendingOrder);
		break;
	case 1:
		ui.videoList->sortItems(Qt::DescendingOrder);
		break;
	}
}

void MainWindow::handleSearch(QString searchString) {
	ui.playlistList->item(0)->setSelected(true);
	vp->searchList(searchString);
	ui.videoList->clear();
	QList<QListWidgetItem*>::iterator it;
	for (it = vp->searchlist.begin(); it != vp->searchlist.end(); ++it)
		ui.videoList->addItem(*it);
	handleSorting();
}

void MainWindow::playlistChanged(QListWidgetItem* item) {
	currentItem = new QString(qvariant_cast<QString>(item->data(Qt::DisplayRole)));
	if (currentItem->compare("Your Library") == 0)
		ui.deletePlaylistButton->setEnabled(false);
	else
		ui.deletePlaylistButton->setEnabled(true);
	updateVideoList();
}

void MainWindow::changeAspectRatio() {
	QList<QAction *> actions = aspectRatioGroup->actions();
	for (auto a : actions) {
		if (a->isChecked() && a->text().compare("Default") == 0)
			ui.video->setAspectRatio(ui.video->defaultAspectRatio());
		else if (a->isChecked() && a->text().compare("16:9") == 0)
			ui.video->setAspectRatio(Vlc::Ratio::R_16_9);
		else if (a->isChecked() && a->text().compare("16:10") == 0)
			ui.video->setAspectRatio(Vlc::Ratio::R_16_10);
		else if (a->isChecked() && a->text().compare("4:3") == 0)
			ui.video->setAspectRatio(Vlc::Ratio::R_4_3);
	}
}

void MainWindow::changeCropRatio() {
	QList<QAction *> actions = cropRatioGroup->actions();
	for (auto a : actions) {
		if (a->isChecked() && a->text().compare("Default") == 0)
			ui.video->setCropRatio(ui.video->defaultCropRatio());
		else if (a->isChecked() && a->text().compare("16:9") == 0)
			ui.video->setCropRatio(Vlc::Ratio::R_16_9);
		else if (a->isChecked() && a->text().compare("16:10") == 0)
			ui.video->setCropRatio(Vlc::Ratio::R_16_10);
		else if (a->isChecked() && a->text().compare("4:3") == 0)
			ui.video->setCropRatio(Vlc::Ratio::R_4_3);
	}
}

void MainWindow::showVideoMenu(const QPoint& pos) {
	QPoint globalPos = ui.video->mapToGlobal(pos);
	mainMenu->exec(globalPos);
}

void MainWindow::updateMenus(QList<QAction *> list, const Vlc::ActionsType type) {
	QList<QAction *>::iterator i;
	for (i = list.begin(); i != list.end(); ++i) {
		if (type == Vlc::ActionsType::VideoTrack) {
			QAction *act = *i;
			if (act->isEnabled())
				act->setEnabled(true);
			else
				act->setEnabled(false);
			videoGroup->addAction(act);
			videoMenu->addAction(act);
		} else if (type == Vlc::ActionsType::AudioTrack) {
			QAction *act = *i;
			if (act->isEnabled())
				act->setEnabled(true);
			else
				act->setEnabled(false);
			audioGroup->addAction(act);
			audioMenu->addAction(act);
		} else if (type == Vlc::ActionsType::Subtitles) {
			QAction *act = *i;
			if (act->isEnabled())
				act->setEnabled(true);
			else
				act->setEnabled(false);
			subtitlesGroup->addAction(act);
			subtitlesMenu->addAction(act);
		}
	}
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event) {
	if (ui.stackedWidget->currentIndex() == 1) {
		if (watched == ui.video) {
			if (event->type() == QEvent::MouseButtonDblClick) {
				if (!fullscreen)
					setFullscreen(true);
				else
					setFullscreen(false);
				return true;
			} else if (event->type() == QEvent::MouseButtonPress) {
				QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
				switch (mouseEvent->buttons()) {
				case Qt::LeftButton:
					if (!ui.pauseButton->isEnabled())
						on_playButton_clicked();
					else if (ui.pauseButton->isEnabled() && ui.stopButton->isEnabled())
						on_pauseButton_clicked();
					return true;
				case Qt::RightButton:
					showVideoMenu(mouseEvent->pos());
					return true;
				}
			} else if (event->type() == QEvent::MouseMove) {
				QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
				QDesktopWidget dw;
				QRect mainScreenSize = dw.availableGeometry(dw.primaryScreen());
				if (fullscreen) {
					if (QCursor::pos().y() > (ui.video->height() - 50)) {
						ui.playerControl->setVisible(true);
					} else {
						ui.playerControl->setVisible(false);
					}
				}
				return true;
			}
		}
	}
	return false;
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
	if (ui.stackedWidget->currentIndex() == 1) {
		switch (event->key()) {
		case Qt::Key_Escape:
			if (fullscreen)
				setFullscreen(false);
		}
	}
}

void MainWindow::updateVideoList() {
	ui.searchEdit->clear();
	vp->updateList(qvariant_cast<QString>(ui.playlistList->selectedItems().at(0)->data(Qt::DisplayRole)));
	ui.videoList->clear();
	QList<QListWidgetItem*>::iterator it;
	for (it = vp->videolist.begin(); it != vp->videolist.end(); ++it)
		ui.videoList->addItem(*it);
	handleSorting();
}

void MainWindow::updatePlaylistList() {
	config->loadPlaylist();
	ui.playlistList->clear();
	allItem = new QListWidgetItem();
	allItem->setData(Qt::DisplayRole, "Your Library");
	allItem->setIcon(QPixmap(":/MainWindow/folder.png"));
	ui.playlistList->addItem(allItem);
	QList<QListWidgetItem*>::iterator it;
	for (it = config->playlists.begin(); it != config->playlists.end(); ++it)
		ui.playlistList->addItem(*it);
	QList<QListWidgetItem*> items = ui.playlistList->findItems(*currentItem, Qt::MatchExactly);
	if (items.size() > 0) {
		ui.playlistList->setItemSelected(items.at(0), true);
	} else {
		ui.playlistList->item(0)->setSelected(true);
	}
}

void MainWindow::setFullscreen(bool f) {
	if (f) {
		fullscreen = true;
		if (windowState() == Qt::WindowMaximized)
			maximized = true;
		else
			maximized = false;
		showFullScreen();
		ui.backButton->hide();
		ui.playerControl->layout()->setContentsMargins(9, 9, 9, 9);
		ui.verticalLayout->removeWidget(ui.playerControl);
		ui.playerWidget->layout()->setContentsMargins(0, 0, 0, 0);
		ui.playerControl->setAutoFillBackground(true);
		ui.playerControl->setVisible(false);
	} else {
		fullscreen = false;
		if (maximized)
			showMaximized();
		else
			showNormal();
		ui.backButton->show();
		ui.verticalLayout->addWidget(ui.playerControl);
		ui.playerWidget->layout()->setContentsMargins(9, 9, 9, 9);
		ui.playerControl->layout()->setContentsMargins(0, 9, 0, 0);
		ui.playerControl->setAutoFillBackground(false);
		ui.playerControl->setVisible(true);
	}
}