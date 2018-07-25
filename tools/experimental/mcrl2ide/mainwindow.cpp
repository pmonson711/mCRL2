// Author(s): Olav Bunte
// Copyright: see the accompanying file COPYING or copy at
// https://github.com/mCRL2org/mCRL2/blob/master/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include "mainwindow.h"

#include <QDockWidget>
#include <QMenu>
#include <QMenuBar>
#include <QToolBar>
#include <QInputDialog>
#include <QDesktopWidget>
#include <QMessageBox>

MainWindow::MainWindow(QString inputProjectFilePath, QWidget* parent)
    : QMainWindow(parent)
{
  specificationEditor = new CodeEditor(this, true);
  setCentralWidget(specificationEditor);

  settings = new QSettings("mCRL2", "mcrl2ide");

  fileSystem = new FileSystem(specificationEditor, settings, this);
  processSystem = new ProcessSystem(fileSystem);

  setupMenuBar();
  setupToolbar();
  setupDocks();

  processSystem->setConsoleDock(consoleDock);

  findAndReplaceDialog = new FindAndReplaceDialog(specificationEditor, this);
  addPropertyDialog =
      new AddEditPropertyDialog(true, processSystem, fileSystem, this);
  connect(addPropertyDialog, SIGNAL(accepted()), this,
          SLOT(actionAddPropertyResult()));
  /* delete any obsolete property files generated by the dialog */
  connect(addPropertyDialog, SIGNAL(rejected()), fileSystem,
          SLOT(deleteUnlistedPropertyFiles()));

  /* make saving a project only enabled whenever there are changes */
  saveProjectAction->setEnabled(false);
  connect(specificationEditor, SIGNAL(modificationChanged(bool)),
          saveProjectAction, SLOT(setEnabled(bool)));

  /* change the tool buttons to start or abort a tool depending on whether
   *   processes are running */
  for (ProcessType processType : PROCESSTYPES)
  {
    connect(processSystem->getProcessThread(processType),
            SIGNAL(statusChanged(bool, ProcessType)), this,
            SLOT(changeToolButtons(bool, ProcessType)));
  }

  /* reset the propertiesdock when the specification changes */
  connect(specificationEditor->document(), SIGNAL(modificationChanged(bool)),
          propertiesDock, SLOT(resetAllPropertyWidgets()));

  /* set the title of the main window */
  setWindowTitle("mCRL2 IDE - Unnamed project");
  if (settings->contains("geometry"))
  {
    restoreGeometry(settings->value("geometry").toByteArray());
  }
  else
  {
    resize(QSize(QDesktopWidget().availableGeometry(this).width() * 0.5,
                 QDesktopWidget().availableGeometry(this).height() * 0.75));
  }

  /* open a project if a project file is given */
  if (!inputProjectFilePath.isEmpty())
  {
    actionOpenProject(inputProjectFilePath);
  }
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupMenuBar()
{
  /* Create the File menu */
  QMenu* fileMenu = menuBar()->addMenu("File");

  newProjectAction =
      fileMenu->addAction(QIcon(":/icons/new_project.png"), "New Project", this,
                          SLOT(actionNewProject(bool)));
  newProjectAction->setShortcut(QKeySequence::New);

  fileMenu->addSeparator();

  openProjectAction =
      fileMenu->addAction(QIcon(":/icons/open_project.png"), "Open Project",
                          this, SLOT(actionOpenProject()));
  openProjectAction->setShortcut(QKeySequence::Open);

  fileMenu->addSeparator();

  saveProjectAction =
      fileMenu->addAction(QIcon(":/icons/save_project.png"), "Save Project",
                          this, SLOT(actionSaveProject()));
  saveProjectAction->setShortcut(QKeySequence::Save);

  saveProjectAsAction =
      fileMenu->addAction("Save Project As", this, SLOT(actionSaveProjectAs()));
  saveProjectAsAction->setShortcut(
      QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_S));

  fileMenu->addSeparator();

  addPropertyAction =
      fileMenu->addAction(QIcon(":/icons/add_property.png"), "Add Property",
                          this, SLOT(actionAddProperty()));

  /* Create the Edit menu */
  QMenu* editMenu = menuBar()->addMenu("Edit");

  undoAction = editMenu->addAction("Undo", specificationEditor, SLOT(undo()));
  undoAction->setShortcut(QKeySequence::Undo);

  redoAction = editMenu->addAction("Redo", specificationEditor, SLOT(redo()));
  redoAction->setShortcut(QKeySequence::Redo);

  editMenu->addSeparator();

  findAndReplaceAction = editMenu->addAction("Find and Replace", this,
                                             SLOT(actionFindAndReplace()));
  findAndReplaceAction->setShortcut(QKeySequence::Find);

  editMenu->addSeparator();

  cutAction = editMenu->addAction("Cut", specificationEditor, SLOT(cut()));
  cutAction->setShortcut(QKeySequence::Cut);

  copyAction = editMenu->addAction("Copy", specificationEditor, SLOT(copy()));
  copyAction->setShortcut(QKeySequence::Copy);

  pasteAction =
      editMenu->addAction("Paste", specificationEditor, SLOT(paste()));
  pasteAction->setShortcut(QKeySequence::Paste);

  deleteAction =
      editMenu->addAction("Delete", specificationEditor, SLOT(deleteChar()));
  deleteAction->setShortcut(QKeySequence::Delete);

  selectAllAction =
      editMenu->addAction("Select All", specificationEditor, SLOT(selectAll()));
  selectAllAction->setShortcut(QKeySequence::SelectAll);

  /* Create the View Menu (more actions are added in setupDocks())*/
  viewMenu = menuBar()->addMenu("View");

  zoomInAction =
      viewMenu->addAction("Zoom in", specificationEditor, SLOT(zoomIn()));
  zoomInAction->setShortcut(QKeySequence::ZoomIn);

  zoomOutAction =
      viewMenu->addAction("Zoom out", specificationEditor, SLOT(zoomOut()));
  zoomOutAction->setShortcut(QKeySequence::ZoomOut);

  viewMenu->addSeparator();

  /* Create the Tools menu */
  QMenu* actionsMenu = menuBar()->addMenu("Tools");

  parseAction = actionsMenu->addAction(parseStartIcon, parseStartText, this,
                                       SLOT(actionParse()));

  simulateAction = actionsMenu->addAction(simulateStartIcon, simulateStartText,
                                          this, SLOT(actionSimulate()));

  actionsMenu->addSeparator();

  showLtsAction = actionsMenu->addAction(showLtsStartIcon, showLtsStartText,
                                         this, SLOT(actionShowLts()));

  showReducedLtsAction =
      actionsMenu->addAction(showReducedLtsStartIcon, showReducedLtsStartText,
                             this, SLOT(actionShowReducedLts()));

  actionsMenu->addSeparator();

  verifyAllPropertiesAction = actionsMenu->addAction(
      verifyAllPropertiesStartIcon, verifyAllPropertiesStartText, this,
      SLOT(actionVerifyAllProperties()));
}

void MainWindow::setupToolbar()
{
  toolbar = addToolBar("Tools");
  toolbar->setIconSize(QSize(48, 48));

  /* create each toolbar item by adding the actions */
  toolbar->addAction(newProjectAction);
  toolbar->addAction(openProjectAction);
  toolbar->addAction(saveProjectAction);
  toolbar->addSeparator();
  toolbar->addAction(parseAction);
  toolbar->addAction(simulateAction);
  toolbar->addSeparator();
  toolbar->addAction(showLtsAction);
  toolbar->addAction(showReducedLtsAction);
  toolbar->addSeparator();
  toolbar->addAction(addPropertyAction);
  toolbar->addAction(verifyAllPropertiesAction);
}

void MainWindow::setDocksToDefault()
{
  addDockWidget(propertiesDock->defaultArea, propertiesDock);
  addDockWidget(consoleDock->defaultArea, consoleDock);

  propertiesDock->setFloating(false);
  consoleDock->setFloating(false);

  propertiesDock->show();
  consoleDock->show();
}

void MainWindow::setupDocks()
{
  /* instantiate the docks */
  propertiesDock = new PropertiesDock(processSystem, fileSystem, this);
  consoleDock = new ConsoleDock(this);

  /* add toggleable option in the view menu for each dock */
  viewMenu->addAction(propertiesDock->toggleViewAction());
  viewMenu->addAction(consoleDock->toggleViewAction());

  /* place the docks in the default dock layout */
  setDocksToDefault();

  /* add option to view menu to put all docks back to their default layout */
  viewMenu->addSeparator();
  viewMenu->addAction("Revert to default layout", this,
                      SLOT(setDocksToDefault()));
}

void MainWindow::actionNewProject(bool askToSave)
{
  QString projectName = fileSystem->newProject(askToSave);
  /* if successful, change the title and empty the properties dock */
  if (!projectName.isEmpty())
  {
    setWindowTitle(QString("mCRL2 IDE - ").append(projectName));
    propertiesDock->setToNoProperties();
  }
}

void MainWindow::actionOpenProject(QString inputProjectFilePath)
{
  QString projectName = "";
  std::list<Property> properties = {};
  if (inputProjectFilePath.isEmpty())
  {
    fileSystem->openProject(&projectName, &properties);
  }
  else
  {
    fileSystem->openProjectFromFolder(inputProjectFilePath, &projectName,
                                      &properties);
  }

  /* if successful, put the properties in the properties dock and set the window
   *   title */
  if (!(projectName.isEmpty()))
  {
    propertiesDock->setToNoProperties();
    for (Property property : properties)
    {
      propertiesDock->addProperty(property);
    }
    setWindowTitle(QString("mCRL2 IDE - ").append(projectName));
  }
}

void MainWindow::actionSaveProject()
{
  QString projectName = fileSystem->saveProject();
  if (!projectName.isEmpty())
  {
    setWindowTitle(QString("mCRL2 IDE - ").append(projectName));
  }
}

void MainWindow::actionSaveProjectAs()
{
  QString projectName = fileSystem->saveProjectAs();
  if (!projectName.isEmpty())
  {
    setWindowTitle(QString("mCRL2 IDE - ").append(projectName));
  }
}

void MainWindow::actionAddProperty()
{
  /* we require a project to be made if no project has been opened */
  if (!fileSystem->projectOpened())
  {
    QMessageBox msgBox(
        QMessageBox::Information, "Add property",
        "To add a property, it is required to create a project first",
        QMessageBox::Ok, this, Qt::WindowCloseButtonHint);
    msgBox.exec();
    actionNewProject(false);
  }

  /* if successful, allow a property to be added */
  addPropertyDialog->clearFields();
  addPropertyDialog->resetFocus();
  if (fileSystem->projectOpened())
  {
    if (addPropertyDialog->isVisible())
    {
      addPropertyDialog->activateWindow();
      addPropertyDialog->setFocus();
    }
    else
    {
      addPropertyDialog->show();
    }
  }
}

void MainWindow::actionAddPropertyResult()
{
  /* if successful (Add button was pressed), create the new property
   * we don't need to save to file as this is already done by the dialog */
  Property property = addPropertyDialog->getProperty();
  fileSystem->newProperty(property);
  propertiesDock->addProperty(property);
}

void MainWindow::actionFindAndReplace()
{
  if (findAndReplaceDialog->isVisible())
  {
    findAndReplaceDialog->setFocus();
    findAndReplaceDialog->activateWindow();
  }
  else
  {
    findAndReplaceDialog->show();
  }
}

void MainWindow::actionParse()
{
  if (processSystem->isThreadRunning(ProcessType::Parsing))
  {
    processSystem->abortAllProcesses(ProcessType::Parsing);
  }
  else
  {
    processSystem->parseSpecification();
  }
}

void MainWindow::actionSimulate()
{
  if (processSystem->isThreadRunning(ProcessType::Simulation))
  {
    processSystem->abortAllProcesses(ProcessType::Simulation);
  }
  else
  {
    processSystem->simulate();
  }
}

void MainWindow::actionShowLts()
{
  if (processSystem->isThreadRunning(ProcessType::LtsCreation))
  {
    processSystem->abortAllProcesses(ProcessType::LtsCreation);
  }
  else
  {
    lastLtsHasReduction = false;
    processSystem->showLts(LtsReduction::None);
  }
}

void MainWindow::actionShowReducedLts()
{
  if (processSystem->isThreadRunning(ProcessType::LtsCreation))
  {
    processSystem->abortAllProcesses(ProcessType::LtsCreation);
  }
  else
  {
    QStringList reductionNames;
    for (std::pair<const LtsReduction, QString> item : LTSREDUCTIONNAMES)
    {
      if (item.first != LtsReduction::None)
      {
        reductionNames << item.second;
      }
    }

    /* ask the user what reduction to use */
    bool ok;
    QString reductionName = QInputDialog::getItem(
        this, "Show reduced LTS", "Reduction:", reductionNames, 0, false, &ok,
        Qt::WindowCloseButtonHint);

    /* if user pressed ok, create a reduced lts */
    if (ok)
    {
      LtsReduction reduction = LtsReduction::None;
      for (std::pair<const LtsReduction, QString> item : LTSREDUCTIONNAMES)
      {
        if (item.second == reductionName)
        {
          reduction = item.first;
        }
      }

      lastLtsHasReduction = true;
      processSystem->showLts(reduction);
    }
  }
}

void MainWindow::actionVerifyAllProperties()
{
  if (processSystem->isThreadRunning(ProcessType::Verification))
  {
    processSystem->abortAllProcesses(ProcessType::Verification);
  }
  else
  {
    propertiesDock->verifyAllProperties();
  }
}

void MainWindow::changeToolButtons(bool toAbort, ProcessType processType)
{
  switch (processType)
  {
  case ProcessType::Parsing:
    if (toAbort)
    {
      parseAction->setText(parseAbortText);
      parseAction->setIcon(parseAbortIcon);
    }
    else
    {
      parseAction->setText(parseStartText);
      parseAction->setIcon(parseStartIcon);
    }
    break;
  case ProcessType::Simulation:
    if (toAbort)
    {
      simulateAction->setText(simulateAbortText);
      simulateAction->setIcon(simulateAbortIcon);
    }
    else
    {
      simulateAction->setText(simulateStartText);
      simulateAction->setIcon(simulateStartIcon);
    }
    break;
  case ProcessType::LtsCreation:
    if (toAbort)
    {
      if (lastLtsHasReduction)
      {
        showLtsAction->setEnabled(false);
        showReducedLtsAction->setText(showReducedLtsAbortText);
        showReducedLtsAction->setIcon(showReducedLtsAbortIcon);
      }
      else
      {
        showReducedLtsAction->setEnabled(false);
        showLtsAction->setText(showLtsAbortText);
        showLtsAction->setIcon(showLtsAbortIcon);
      }
    }
    else
    {
      showLtsAction->setEnabled(true);
      showLtsAction->setText(showLtsStartText);
      showLtsAction->setIcon(showLtsStartIcon);
      showReducedLtsAction->setEnabled(true);
      showReducedLtsAction->setText(showReducedLtsStartText);
      showReducedLtsAction->setIcon(showReducedLtsStartIcon);
    }
    break;
  case ProcessType::Verification:
    if (toAbort)
    {
      verifyAllPropertiesAction->setText(verifyAllPropertiesAbortText);
      verifyAllPropertiesAction->setIcon(verifyAllPropertiesAbortIcon);
    }
    else
    {
      verifyAllPropertiesAction->setText(verifyAllPropertiesStartText);
      verifyAllPropertiesAction->setIcon(verifyAllPropertiesStartIcon);
    }
    break;
  default:
    break;
  }
}

void MainWindow::closeEvent(QCloseEvent* event)
{
  /* if there are changes, ask the user to save the project first */
  if (fileSystem->isSpecificationModified())
  {
    QMessageBox::StandardButton result = QMessageBox::question(
        this, "mCRL2 IDE",
        "There are changes in the current project, do you want to save?",
        QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
    switch (result)
    {
    case QMessageBox::Yes:
      if (fileSystem->saveProject().isEmpty())
      {
        event->ignore();
      }
      else
      {
        event->accept();
      }
      break;
    case QMessageBox::No:
      event->accept();
      break;
    case QMessageBox::Cancel:
      event->ignore();
      break;
    default:
      break;
    }
  }

  /* save the settings for the main window */
  settings->setValue("geometry", saveGeometry());

  /* remove the temporary folder */
  fileSystem->removeTemporaryFolder();

  /* abort all processes */
  for (ProcessType processType : PROCESSTYPES)
  {
    processSystem->abortAllProcesses(processType);
  }
}
