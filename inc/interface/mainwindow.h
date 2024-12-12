#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QRadioButton>
#include <QScrollArea>
#include <QLCDNumber>
#include <QSlider>
#include <QToolBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QThread>
#include "emulator/kyiv.h"
#include <QTabWidget>
#include <QPushButton>
#include <QTabBar>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE



class MainWindow : public QMainWindow
{
    Q_OBJECT

private:
    Ui::MainWindow *ui;
    Kyiv_t machine;

    QScrollArea* scrollArea;    // main scroll area
    QWidget* widget;
    QVBoxLayout* kyivLay;
    QVBoxLayout* mainLay;
    QHBoxLayout* panelAndCode;

    QVector<QVector<QRadioButton*>> upControl;
    QVector<QVector<QRadioButton*>> lowControl;
    QSlider *slider2 = new QSlider(Qt::Vertical);
    QVector<QVector<QRadioButton*>> rom_buttons;
    QToolBox* toolBox;

    QPushButton *komytatorsBtn;
    QPushButton *rungeKuttaBtn;
    QPushButton *sqrtAndFriendsBtn;

    QWidget *drumsTab;
    QWidget *instructionTab;


    int curr_takt = 0;

private:

    void InitSignalPanel();
    void InitControlPanel();
    void InitROMPanel();
    void InitASMDisASMPanel();
    void createChartTab(const QString & outputText);
    void InitTabs();
    void addNewTab(QTabWidget *tabWidget);

   // QString processAssemblyCode(const QString &inputCode);

  //  QString generateChartFromOutput(const QString &output);

    void InitProgramsPanel();
    void InitPunchCardsPanel();
    //void InitDrumsPanel();
    void InitDrumsPanel(QVBoxLayout *drumsLayout);


public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

public slots:
    void OnSaveButtonClicked();
    void OnAddButtonClicked();
    void OnDelButtonClicked();
    void OnSavePerfoDataButtonClicked();
    void OnOpenDisasmFileButtonClicked();
    void OnAssemblyButtonClicked();
    void OnSaveROMButtonClicked();
    void OnuoButtonClicked();
    void OnOstanovButtonClicked();
    void OnBlockCButtonClicked();
    void OnBlockAButtonClicked();
    void OnSetFromPultButtonClicked();
    void OnOstanovPultButtonClicked();
    void OnBlockOstanovButtonClicked();
    void OnAvarOstTrigButtonClicked();
    void OnSetSpeedClicked();
    void OnKButtonClicked();
    void OnKomytatorsButtonClicked();
    void OnRungeKuttaButtonClicked();
    void OnSqrtAndFriendsButtonClicked();

    void ReadProgram(const std::string& filepath);

    void CreateOrSwitchToDrumsTab(QTabWidget *tabWidget);
//QWidget *drumsTab;       // Pointer to the Drums tab
     // Pointer to the Instruction tab

    // Member Functions
    //void Create//OrSwitchToDrumsTab(QTabWidget *tabWidget);       // Creates or switches to the Drums tab
    void CreateOrSwitchToInstructionTab(QTabWidget *tabWidget);
    void CreateInstructionTab(QTabWidget *tabWidget);
    void CreateDrumsTab(QTabWidget *tabWidget);

};
#endif // MAINWINDOW_H
